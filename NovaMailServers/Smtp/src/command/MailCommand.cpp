#include <command/MailCommand.hpp>
#include <Session.hpp>
#include <Transaction.hpp>
#include <types/Types.hpp>
#include <address/EmailAddress.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <core/Utils.hpp>

namespace NovaMail {
    namespace Smtp {
        // 处理 MAIL FROM 参数
        static bool HandleMailFromParam(const CommandParam& param, Types::MailFrom& mailFrom, std::string_view rawLine) {
            bool hasFromValue  = false;
            for (const ValuePart& value : param.values) {
                switch (value.role) {
                case ValueRole::Address: {
                    hasFromValue = true;
                    // MAIL FROM:<> 空回信路径
                    if (value.text.empty()) {
                        mailFrom.SetNullReversePath(true);
                        return true;
                    }

                    Address::EmailAddress address;
                    if (!Address::EmailAddress::Parse(value.text, address)) {
                        LogWarn(u"Invalid MAIL FROM address '{}', command line: {}", value.text, std::string(rawLine));
                        return false;
                    }

                    mailFrom.SetAddress(address);
                    mailFrom.SetNullReversePath(false);
                } break;
                case ValueRole::DisplayName:
                    mailFrom.SetDisplayName(value.text); break;
                default: LogWarn(
                        u"Unexpected value role '{}' in MAIL FROM parameter, command line: {}",
                        std::to_string(static_cast<int>(value.role)), std::string(rawLine)
                    ); break;
                }
            }
            if (!hasFromValue ) {
                LogWarn(u"MAIL FROM parameter missing address, command line: {}", std::string(rawLine));
                return false;
            }
            return true;
        }

        // 尝试解析 size_t，返回 false 表示解析失败（非数字或溢出）
        static bool TryParseSizeT(std::string_view s, size_t& out) noexcept {
            if (s.empty()) return false;
            size_t value = 0;
            for (char c : s) {
                if (c < '0' || c > '9') return false;
                const size_t digit = static_cast<size_t>(c - '0');
                // 简单溢出保护
                if (value > (std::numeric_limits<size_t>::max() - digit) / 10) return false;
                value = value * 10 + digit;
            }
            out = value;
            return true;
        }

        // 处理 MAIL SIZE 参数
        static bool HandleMailSizeParam(const CommandParam& param, Types::MailFrom& mailFrom, std::string_view rawLine) {
            bool hasValue = false;
            for (const ValuePart& value : param.values) {
                if (value.role != ValueRole::Item) {
                    LogWarn(
                        u"Unexpected value role '{}' in MAIL SIZE parameter, command line: {}",
                        std::to_string(static_cast<int>(value.role)), std::string(rawLine)
                    ); continue;
                }
                hasValue = true;
                size_t mailSize = 0;
                if (!TryParseSizeT(value.text, mailSize)) {
                    LogWarn( u"Invalid SIZE value '{}' in MAIL command, command line: {}", value.text, std::string(rawLine));
                    return false;
                }
                mailFrom.SetSize(mailSize);
            }
            if (!hasValue) {
                LogWarn(u"MAIL SIZE parameter missing value, command line: {}", std::string(rawLine));
                return false;
            }
            return true;
        }

        // 处理 MAIL BODY 参数
        static bool HandleMailBodyParam(const CommandParam& param, Types::MailFrom& mailFrom, std::string_view rawLine) {
            bool hasValue = false;
            for (const ValuePart& value : param.values) {
                if (value.role != ValueRole::Item) {
                    LogWarn(
                        u"Unexpected value role '{}' in MAIL BODY parameter, command line: {}",
                        std::to_string(static_cast<int>(value.role)), std::string(rawLine)
                    ); continue;
                }
                hasValue = true;
                mailFrom.SetBody(value.text);
            }
            if (!hasValue) {
                LogWarn(u"MAIL BODY parameter missing value, command line: {}", std::string(rawLine));
                return false;
            }
            return true;
        }

        void MailCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;
            // 获取当前状态
            Types::State state = session->GetState();

            // 未完成问候/识别
            if (state == Types::State::Connected || state == Types::State::Greeted) {
                LogWarn(u"MAIL command before EHLO/HELO, command line: {}", std::string(command.RawLine()));
                session->Reply(Types::ReplyCode::BadSequence, "Bad sequence of commands, send HELO/EHLO first");
                return;
            }

            // 当前会话要求认证后才能提交邮件，例如 587 Submission / 中继模式
            if (session->RelayPolicy() && !session->Authenticated()) {
                LogWarn(
                    u"MAIL command rejected: authentication required, command line: {}, client={} -> server={}",
                    std::string(command.RawLine()), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                );
                session->Reply(Types::ReplyCode::AuthenticationRequired, "Authentication required");
                return;
            }

            // 允许开始/覆盖一个新的事务
            bool hasFrom = false;
            Transaction& transaction = session->GetTransaction();
            Types::MailFrom& mailFrom = transaction.GetMailFrom();
            for (const CommandParam& param : command.Params()) {
                if (param.kind != ParamKind::KeyValue) {
                    LogWarn(
                        u"Unexpected positional parameter '{}' in MAIL command, command line: {}",
                        param.raw, std::string(std::string(command.RawLine()))
                    ); continue;
                }
            
                // 解析 MAIL FROM 参数
                if (Core::Utils::IEquals(param.key, "FROM")) {
                    if (!HandleMailFromParam(param, mailFrom, std::string(command.RawLine()))) {
                        LogError(u"Invalid MAIL FROM parameter, command line: {}", std::string(std::string(command.RawLine())));
                        session->Reply(Types::ReplyCode::ParamError, "Syntax error in parameters or arguments");
                        return;
                    }
                    hasFrom = true;
                    continue;
                }
            
                // 解析 SIZE 参数
                if (Core::Utils::IEquals(param.key, "SIZE")) {
                    if (!HandleMailSizeParam(param, mailFrom, std::string(command.RawLine()))) {
                        LogError(u"Invalid MAIL SIZE parameter, command line: {}", std::string(std::string(command.RawLine())));
                        session->Reply(Types::ReplyCode::ParamError, "Syntax error in parameters or arguments, invalid SIZE parameter");
                        return;
                    }
                    if (mailFrom.MailSize() > transaction.MaxDataBytes()) {
                        LogWarn(
                            u"MAIL SIZE parameter {} exceeds server limit {}, command line: {}",
                            std::to_string(mailFrom.MailSize()), std::to_string(transaction.MaxDataBytes()), std::string(std::string(command.RawLine()))
                        );
                        session->Reply(Types::ReplyCode::MessageTooLarge, "Message size exceeds fixed maximum message size");
                        return;
                    }
                    continue;
                }

                // 解析 BODY 参数
                if (Core::Utils::IEquals(param.key, "BODY")) {
                    if (!HandleMailBodyParam(param, mailFrom, std::string(command.RawLine()))) {
                        LogError(u"Invalid MAIL BODY parameter, command line: {}", std::string(std::string(command.RawLine())));
                        session->Reply(Types::ReplyCode::ParamError, "Syntax error in parameters or arguments, invalid BODY parameter");
                        return;
                    }
                    
                    switch (mailFrom.MailBodyType())
                    {
                    case Types::MailBody::Unspecified:
                    case Types::MailBody::Bit7: break;
                    case Types::MailBody::Bit8Mime: {
                        Types::Capabilities caps = session->GetCapabilities();
                        if (!Types::HasCapability(caps, Types::Capabilities::EIGHT_BIT_MIME)) {
                            LogWarn(u"Client requested BODY=8BITMIME but server does not advertise 8BITMIME");
                            session->Reply(Types::ReplyCode::ParamUnsupported, "BODY type not supported");
                            return;
                        }
                    } break;
                    case Types::MailBody::BinaryMime: {
                        Types::Capabilities caps = session->GetCapabilities();
                        if (!Types::HasCapability(caps, Types::Capabilities::BINARY_MIME)
                        && !Types::HasCapability(caps, Types::Capabilities::CHUNKING)) {
                            LogWarn(u"Client requested BODY=BINARYMIME but server lacks CHUNKING/BINARYMIME capability");
                            session->Reply(Types::ReplyCode::ParamUnsupported, "BINARYMIME not supported");
                            return;
                        }
                    } break;
                    }
                    continue;
                }
                LogWarn(u"Unrecognized parameter '{}' in MAIL command, command line: {}", param.key, std::string(std::string(command.RawLine())));
            }

            // MAIL FROM 没出现过
            if (!hasFrom) {
                LogError(u"MAIL command missing valid FROM address, command line: {}", std::string(std::string(command.RawLine())));
                session->Reply(Types::ReplyCode::ParamError, "MAIL command requires FROM parameter");
                return;
            }

            // 客户端是否完成认证
            if (session->Authenticated()) { // 已认证
                // 验证发件人地址
                NovaMail::Smtp::Entity::MailBox mailBox;
                Service::MailBoxService::GetMailBoxResult getMailBoxResult = m_mailBoxService.GetMailBox(mailFrom.Address(), mailBox, session->Authenticated() ? session->GetUser()->userId : 0);
                if (getMailBoxResult == Service::MailBoxService::GetMailBoxResult::Normal) { // 邮箱 成功得到
                    mailFrom.SetMailBox(mailBox);
                    // 标记：来源为站内
                    mailFrom.SetSourceType(Types::MailSourceType::Local);
                } else if(getMailBoxResult == Service::MailBoxService::GetMailBoxResult::Disabled) { // 邮箱被禁用
                    LogWarn(
                        u"MAIL FROM rejected: mailbox {} disabled or user inactive, client={} -> server={}",
                        mailFrom.Address().ToString(),
                        session->GetRemoteAddress().ToString(),
                        session->GetLocalAddress().ToString()
                    );
                    session->Reply(Types::ReplyCode::MailboxUnavailable, "<" + mailFrom.Address().ToString() + ">: Sender address rejected: Mailbox disabled");
                    return;
                } else if (getMailBoxResult == Service::MailBoxService::GetMailBoxResult::NotOwned) { // 邮箱不属于该用户
                    LogWarn(
                        u"MAIL FROM rejected: mailbox {} not owned or user inactive, client={} -> server={}",
                        mailFrom.Address().ToString(), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                    );
                    session->Reply(Types::ReplyCode::MailboxNameNotAllowed, "<" + mailFrom.Address().ToString() + ">: Sender address rejected: not owned by user");
                    return;
                } else {
                    LogWarn(
                        u"MAIL FROM rejected: mailbox {} user inactive, client={} -> server={}",
                        mailFrom.Address().ToString(), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                    );
                    session->Reply(Types::ReplyCode::MailboxUnavailable, "<" + mailFrom.Address().ToString() + ">: Sender address rejected");
                    return;
                }
            } else {
                // 未认证
                if (mailFrom.NullReversePath()) {
                    // MAIL FROM:<>，退信场景
                    mailFrom.SetSourceType(Types::MailSourceType::NullReversePath);
                } else if (m_mailBoxService.IsLocalDomain(mailFrom.Address().GetDomain())) {
                    LogWarn(
                        u"MAIL FROM rejected: mailbox {} authentication required for local sender, client={} -> server={}",
                        mailFrom.Address().ToString(), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                    );
                    // 可能是恶意客户端试图伪造本地邮箱
                    session->Reply(Types::ReplyCode::MailboxNameNotAllowed, "<" + mailFrom.Address().ToString() + ">: Sender address rejected: authentication required for local sender");
                    return;
                } else {
                    // 标记：来源为外来
                    mailFrom.SetSourceType(Types::MailSourceType::External);
                }
            }
            session->SetState(Types::State::MailStarted); // 切换状态
            session->Reply(Types::ReplyCode::Ok, "OK");
        }
    }
}