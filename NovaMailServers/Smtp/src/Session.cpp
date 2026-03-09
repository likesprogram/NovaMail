#include <Session.hpp>
#include <iostream>
#include <LikeHub/LikeHubConfig.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Smtp {
        Session::Session() {
            // 获取配置
            LikeHub::LikeHubConfig config = LikeHub::LikeHubConfig::Instance();
            m_config = config.Raw();

            // EHLO 广播支持
            m_caps = Capabilities::PIPELINING | Capabilities::SIZE | Capabilities::EIGHT_BIT_MIME | Capabilities::ENHANCED_STATUS;

            // 创建 Transaction
            size_t maxRcptCount = static_cast<size_t>(m_config[u"transaction"][u"maxRcptCount"].AsInt64());
            size_t maxDataBytes = static_cast<size_t>(m_config[u"transaction"][u"maxDataBytes"].AsInt64());
            bool dedupRcpt = m_config[u"transaction"][u"dedupRcpt"].AsBool();
            m_tx = Transaction(maxRcptCount, maxDataBytes, dedupRcpt);

            // 输出日志
            LogDebug(
                u"Session created with maxRcptCount={}, maxDataBytes={}, dedupRcpt={}.",
                std::to_string(maxRcptCount),
                std::to_string(maxDataBytes), dedupRcpt ? u"true" : u"false"
            );
        }

        void Session::OnConnect(LikesProgram::Net::Buffer& out) {
            std::string message = "220 ";
            message.append(m_config[u"hostname"].AsString().ToStdString());
            message.append(" ");
            message.append(m_config[u"greeting"].AsString().ToStdString());
            message.append(".\r\n");
            out.Append(message.c_str(), message.size());
            m_state = State::Greeted; // 220 发送完成，等待 问候

            // 输出日志
            LogInfo(
                u"SMTP client connected, server={}, greeting={}",
                m_config[u"hostname"].AsString(),
                m_config[u"greeting"].AsString()
            );
        }

        void Session::OnBytes(LikesProgram::Net::Buffer& in, LikesProgram::Net::Buffer& out) {
            for (;;) {
                if (m_state == State::DataReceiving) {
                    if (!PumpData(in, out)) break;   // 返回 false 表示需要更多数据
                    // 判断是否结束
                    continue;                        // data 完成后继续处理可能的 pipeline 命令
                }

                // 尝试解析指令
                auto cmd = TryParseCommand(in);      // optional<Command>
                if (!cmd) break;                     // 没有完整命令行

                // 派遣指令处理
                Dispatch(*cmd, out);

                // 已关闭
                if (m_shouldClose) break;
            }
        }

        bool Session::ShouldClose() const noexcept { 
            return m_shouldClose; 
        }

        void Session::OnEhlo(const Command& command, LikesProgram::Net::Buffer& out) {
            (void)command;
            std::string message = "250-";
            message.append(m_config[u"hostname"].AsString().ToStdString());
            message.append(" ");
            message.append(m_config[u"greeting"].AsString().ToStdString());
            message.append("\r\n");
            if (m_caps != Capabilities::None) {
                // if((m_caps & Capabilities::ESMTP) != Capabilities::None) {
                //     message.append("250-ESMTP\r\n");
                // }
                if((m_caps & Capabilities::PIPELINING) != Capabilities::None) {
                    message.append("250-PIPELINING\r\n");
                }
                if((m_caps & Capabilities::SIZE) != Capabilities::None) {
                    message.append("250-SIZE ");
                    message.append(m_config[u"transaction"][u"maxDataBytes"].AsString().ToStdString());
                    message.append("\r\n");
                }
                if ((m_caps & Capabilities::ENHANCED_STATUS) != Capabilities::None) {
                    message.append("250-ENHANCEDSTATUSCODES\r\n");
                }
                if ((m_caps & Capabilities::DSN) != Capabilities::None) {
                    message.append("250-DSN\r\n");
                }
                if((m_caps & Capabilities::STARTTLS) != Capabilities::None && !m_isTLS) {
                    message.append("250-STARTTLS\r\n");
                }
                if ((m_caps & Capabilities::AUTH) != Capabilities::None) {
                    message.append("250-AUTH");
                    if ((m_caps & Capabilities::AUTH_PLAIN) != Capabilities::None) message.append(" PLAIN");
                    if ((m_caps & Capabilities::AUTH_LOGIN) != Capabilities::None) message.append(" LOGIN");
                    if ((m_caps & Capabilities::AUTH_CRAM_MD5) != Capabilities::None) message.append(" CRAM-MD5");
                    message.append("\r\n");
                }
                if ((m_caps & Capabilities::EIGHT_BIT_MIME) != Capabilities::None) {
                    message.append("250-8BITMIME\r\n");
                }
                // if ((m_caps & Capabilities::BINARY_MIME) != Capabilities::None) {
                //     message.append("250-BINARYMIME\r\n");
                //     message.append("250-CHUNKING\r\n");
                // }
                if ((m_caps & Capabilities::SMTP_UTF8) != Capabilities::None) {
                    message.append("250-SMTPUTF8\r\n");
                }
                // if ((m_caps & Capabilities::VRFY) != Capabilities::None) {
                //     message.append("250-VRFY\r\n");
                // }
                // if ((m_caps & Capabilities::EXPN) != Capabilities::None) {
                //     message.append("250-EXPN\r\n");
                // }
                if ((m_caps & Capabilities::HELP) != Capabilities::None) {
                    message.append("250-HELP\r\n");
                }
            }

            // 记录 EHLO 参数（客户端域名）
            if (command.Params().size() > 0 && command.Params().front().values.size() > 0)
                m_tx.SetEhloDomain(command.Params().front().values.front().text);

            // 回复 250，EHLO 完成
            message.append("250 OK\r\n");
            m_state = State::Identified; // EHLO/HELO 完成
            out.Append(message.c_str(), message.size());

            // 输出日志
            LogInfo(
                u"EHLO from clientDomain={}",
                m_tx.HasEhloDomain() ? std::string(m_tx.EhloDomain()) : std::string("unknown")
            );
            LogDebug(
                u"SMTP capabilities advertised: PIPELINING={} SIZE={} 8BITMIME={}",
                (m_caps & Capabilities::PIPELINING) != Capabilities::None,
                (m_caps & Capabilities::SIZE) != Capabilities::None,
                (m_caps & Capabilities::EIGHT_BIT_MIME) != Capabilities::None
            );
        }

        void Session::OnMail(const Command& command, LikesProgram::Net::Buffer& out) {
            // DATA 阶段不能开始新事务
            if (m_state == State::DataReceiving) {
                Reply503(out);
                LogError(u"Received MAIL command during DATA receiving phase.");
                return;
            }

            // 未完成问候/识别
            if (m_state == State::Connected || m_state == State::Greeted) {
                Reply503(out);
                LogWarn(u"Received MAIL command before EHLO/HELO.");
                return;
            }

            // 允许开始/覆盖一个新的事务：隐式 RSET
            if (command.Params().size() > 0 && command.Params().front().values.size() > 0) {
                m_tx.SetMailFrom(command.Params().front().values.front().text);
            } else {
                Reply501(out);
                LogError(u"MAIL command missing FROM parameter, command line: {}", std::string(command.RawLine()));
                return;
            }

            m_state = State::MailStarted; // 切换状态
            static constexpr char kOk[] = "250 Ok\r\n";
            out.Append(kOk, sizeof(kOk) - 1);

            // 输出日志
            LogInfo(
                u"MAIL FROM accepted: {}",
                std::string(m_tx.MailFrom())
            );
        }

        void Session::OnRcpt(const Command& command, LikesProgram::Net::Buffer& out) {
            // DATA 阶段不能开始新事务
            if (m_state == State::DataReceiving) {
                Reply503(out);
                LogError(u"Received RCPT command during DATA receiving phase.");
                return;
            }
        
            // 未完成问候/识别/未完成 MAIL
            if (m_state == State::Connected || m_state == State::Greeted || m_state == State::Identified) {
                Reply503(out);
                LogWarn(u"Received RCPT command before EHLO/HELO or MAIL FROM.");
                return;
            }
        
            // 记录 RCPT 参数（收件人地址）
            if (command.Params().size() > 0 && command.Params().front().values.size() > 0) {
                std::string_view rcpt = command.Params().front().values.front().text;
                if (!m_tx.AddRcptTo(rcpt)) {
                    Reply501(out);
                    if (m_tx.RcptCount() >= m_tx.MaxRcptCount()) LogWarn(u"RCPT command exceeds maximum recipient count {}, command line: {}", std::to_string(m_tx.MaxRcptCount()), std::string(command.RawLine()));
                    if (rcpt.empty()) LogError(u"RCPT command has empty TO parameter, command line: {}", std::string(command.RawLine()));
                    else LogError(u"Failed to add RCPT TO address '{}', command line: {}", std::string(rcpt), std::string(command.RawLine()));
                    return;
                }

                LogDebug(
                    u"RCPT TO accepted: {} (total={})",
                    std::string(rcpt),
                    std::to_string(m_tx.RcptCount())
                );
            } else {
                Reply501(out);
                LogError(u"RCPT command missing TO parameter, command line: {}", std::string(command.RawLine()));
                return;
            }
        
            m_state = State::RcptCollecting; // 切换状态
            static constexpr char kOk[] = "250 Ok\r\n";
            out.Append(kOk, sizeof(kOk) - 1);
        }

        void Session::OnData(LikesProgram::Net::Buffer& out) {
            // DATA 阶段不能开始新事务
            if (m_state == State::DataReceiving) {
                Reply503(out);
                LogError(u"Received DATA command during DATA receiving phase.");
                return;
            }

            // 未完成问候/识别/未完成 MAIL/未完成 RCPT
            if (m_state == State::Connected || m_state == State::Greeted || m_state == State::Identified || m_state != State::RcptCollecting) {
                Reply503(out);
                LogWarn(u"Received DATA command before EHLO/HELO, MAIL FROM or RCPT TO.");
                return;
            }

            static constexpr char k354[] = "354 End data with <CR><LF>.<CR><LF>\r\n";
            out.Append(k354, sizeof(k354) - 1);

            // 清空上一封邮件缓存
            m_tx.ResetData();

            m_state = State::DataReceiving;

            std::string recipients;
            for (const auto& rcpt : m_tx.RcptTos()) {
                recipients.append(rcpt);
                recipients.append(" ");
                std::cout << "Recipient: " << rcpt << std::endl; // 输出收件人地址
            }
            LogInfo(
                u"DATA begin: from={} rcptCount={} recipients: {}",
                std::string(m_tx.MailFrom()),
                std::to_string(m_tx.RcptCount()),
                recipients.empty() ? std::string("none") : recipients
            );
        }

        void Session::OnRset(LikesProgram::Net::Buffer& out) {
            m_tx.Reset(); // 清理缓存
            if (m_state >= State::Identified) {
                m_state = State::Identified;
            } else {
                m_state = State::Greeted; // 还没 EHLO
            }
            
            static constexpr char kOut[] = "250 OK\r\n";
            out.Append(kOut, sizeof(kOut) - 1);
        }

        void Session::OnNoop(LikesProgram::Net::Buffer& out) {
            static constexpr char kOut[] = "250 OK\r\n";
            out.Append(kOut, sizeof(kOut) - 1);
        }

        void Session::OnQuit(LikesProgram::Net::Buffer& out) {
            static constexpr char kQuit[] = "221 Nova Mail Service closing transmission channel\r\n";
            out.Append(kQuit, sizeof(kQuit) - 1);

            LogInfo(
                u"SMTP client quit: ehloDomain={}",
                m_tx.HasEhloDomain() ? std::string(m_tx.EhloDomain()) : std::string("unknown")
            );

            m_shouldClose = true;
        }

        void Session::Reply500(LikesProgram::Net::Buffer& out) {
            static constexpr char kOut[] = "500 Syntax error, command unrecognized.\r\n";
            out.Append(kOut, sizeof(kOut) - 1);
        }

        void Session::Reply501(LikesProgram::Net::Buffer& out) {
            static constexpr char kOut[] = "501 Syntax error in parameters.\r\n";
            out.Append(kOut, sizeof(kOut) - 1);
        }

        void Session::Reply502(LikesProgram::Net::Buffer& out) {
            static constexpr char kOut[] = "502 Command not implemented.\r\n";
            out.Append(kOut, sizeof(kOut) - 1);
        }

        void Session::Reply503(LikesProgram::Net::Buffer& out) {
            static constexpr char kOut[] = "503 Bad sequence of commands.\r\n";
            out.Append(kOut, sizeof(kOut) - 1);
        }

        void Session::HandleData(LikesProgram::Net::Buffer& in, LikesProgram::Net::Buffer& out) {
            size_t consumeSize = 0;
            bool exceed = m_tx.AppendData(in.Peek(), in.ReadableBytes(), consumeSize);
            in.Consume(consumeSize); // 消费
            if (!exceed) { // 超限
                static constexpr char kErr[] = "552 Message size exceeds fixed maximum message size.\r\n";
                out.Append(kErr, sizeof(kErr) - 1);
                m_tx.Reset(); // 清除缓存
                m_state = State::Identified;
                LogWarn(
                    u"SMTP message too large: size={} limit={}",
                    std::to_string(m_tx.DataBytes()),
                    std::to_string(m_tx.MaxDataBytes())
                );
                return;
            }

            if (m_tx.IsDataComplete()) {
                static constexpr char kErr[] = "250 OK\r\n";
                out.Append(kErr, sizeof(kErr) - 1);
                m_state = State::DataReceiptCompleted; // 测试状态
                
                LikesProgram::Net::Buffer& mailData = m_tx.Data();

                LogInfo(
                    u"SMTP message received: from={} rcptCount={} size={}",
                    std::string(m_tx.MailFrom()),
                    std::to_string(m_tx.RcptCount()),
                    std::to_string(m_tx.DataBytes())
                );

                LogTrace(u"SMTP raw message size={}", std::to_string(mailData.ReadableBytes()));
                
                // 测试
                // std::cout << std::string_view(reinterpret_cast<const char*>(mailData.Peek()), mailData.ReadableBytes()) << std::endl;
            }

            /* TODO :
                1、验证 DKIM (数据库中 "来源" 字段需要)
                2、规范化邮件 (添加Message-ID、添加 X-Internal-ID等)
                5、保存到文件服务器 (MinIO)
                6、写入数据库
            */
        }

        bool Session::PumpData(Buffer& in, Buffer& out) {
            HandleData(in, out);
            if (m_state == State::DataReceiving) return false; // 需要更多数据
            return true; // data 已完成（成功/失败），可继续解析下一条命令
        }
        
        std::optional<Command> Session::TryParseCommand(LikesProgram::Net::Buffer& in) {
            Command outCommand(in); // 若是有一行则会自动消费，没有则不做任何处理
            if (outCommand.NoComplete()) return std::nullopt;
            return outCommand;
        }

        void Session::Dispatch(const Command& command, LikesProgram::Net::Buffer& out) {
            LogDebug(u"SMTP command received: {}", std::string(command.RawLine()));
            switch (command.Verb()) {
                case CommandVerb::EHLO:
                case CommandVerb::HELO: OnEhlo(command, out); break;
                case CommandVerb::MAIL: OnMail(command, out); break;
                case CommandVerb::RCPT: OnRcpt(command, out); break;
                case CommandVerb::DATA: OnData(out); break;
                case CommandVerb::RSET: OnRset(out); break;
                case CommandVerb::NOOP: OnNoop(out); break;
                case CommandVerb::QUIT: OnQuit(out); break;
                case CommandVerb::Unknown: Reply500(out); break; // 不认识
                default: Reply502(out); break; // 认识但是不支持
            }
        }
    }
}