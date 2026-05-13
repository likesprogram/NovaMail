#include <Session.hpp>
#include <Connection.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <command/EhloCommand.hpp>
#include <command/StartTlsCommand.hpp>
#include <command/AuthCommand.hpp>
#include <command/MailCommand.hpp>
#include <command/RcptCommand.hpp>
#include <command/DataCommand.hpp>
#include <command/RsetCommand.hpp>
#include <command/NoopCommand.hpp>
#include <command/QuitCommand.hpp>
#include <service/MailBoxService.hpp>
#include <LikeHub/SecurityContextManager.hpp>

namespace NovaMail {
    namespace Smtp {
        Session::Session(Connection* connection, LikesProgram::Net::Address remoteAddr, LikesProgram::Net::Address localAddr)
        : m_connection(connection), m_remoteAddr(remoteAddr), m_localAddr(localAddr) {
            // 获取配置
            LikeHub::LikeHubConfig& config = LikeHub::LikeHubConfig::Instance();
            m_config = config.Raw();

            // 获取服务名称
            m_smtpHostname = m_config[u"smtp"][u"hostname"].AsString().ToStdString();

            // EHLO 广播支持
            m_caps = Types::Capabilities::PIPELINING | Types::Capabilities::SIZE | Types::Capabilities::EIGHT_BIT_MIME;
            if (m_config[u"smtp"][u"start_tls"].AsBool()) m_caps |= Types::Capabilities::STARTTLS;
            if (m_config[u"smtp"][u"relay_policy"].AsBool()) { // 开启中继 必须完成登录认证 才能发送
                m_relayPolicy = true;
                m_caps |= Types::Capabilities::STARTTLS; // 开启登录前必须开启 TLS 通信
                m_caps |= Types::Capabilities::AUTH | Types::Capabilities::AUTH_PLAIN | Types::Capabilities::AUTH_LOGIN;
            }

            // 创建 Transaction
            size_t maxRcptCount = static_cast<size_t>(m_config[u"transaction"][u"maxRcptCount"].AsInt64());
            size_t maxDataBytes = static_cast<size_t>(m_config[u"transaction"][u"maxDataBytes"].AsInt64());
            bool dedupRcpt = m_config[u"transaction"][u"dedupRcpt"].AsBool();
            m_tx = Transaction(maxRcptCount, maxDataBytes, dedupRcpt);
            InitHandlers();

            // 输出日志
            LogDebug(
                u"Session created with maxRcptCount={}, maxDataBytes={}, dedupRcpt={}",
                std::to_string(maxRcptCount),
                std::to_string(maxDataBytes), dedupRcpt ? u"true" : u"false"
            );
        }

        void Session::InitHandlers() {
            m_handlers[static_cast<size_t>(Types::CommandVerb::EHLO)] = std::make_unique<EhloCommand>(
                m_smtpHostname, m_config[u"smtp"][u"greeting"].AsString().ToStdString()
            );
            m_handlers[static_cast<size_t>(Types::CommandVerb::HELO)] = std::make_unique<EhloCommand>(
                m_smtpHostname, m_config[u"smtp"][u"greeting"].AsString().ToStdString()
            );
            m_handlers[static_cast<size_t>(Types::CommandVerb::STARTTLS)] = std::make_unique<StartTlsCommand>();
            m_handlers[static_cast<size_t>(Types::CommandVerb::AUTH)] = std::make_unique<AuthCommand>();
            m_handlers[static_cast<size_t>(Types::CommandVerb::MAIL)] = std::make_unique<MailCommand>();
            m_handlers[static_cast<size_t>(Types::CommandVerb::RCPT)] = std::make_unique<RcptCommand>();
            m_handlers[static_cast<size_t>(Types::CommandVerb::DATA)] = std::make_unique<DataCommand>();

            m_handlers[static_cast<size_t>(Types::CommandVerb::RSET)] = std::make_unique<RsetCommand>();
            m_handlers[static_cast<size_t>(Types::CommandVerb::NOOP)] = std::make_unique<NoopCommand>();
            m_handlers[static_cast<size_t>(Types::CommandVerb::QUIT)] = std::make_unique<QuitCommand>();
        }

        void Session::OnConnect() {
            if (ShouldClose()) return;
            std::string message = "";
            message.append(m_config[u"smtp"][u"hostname"].AsString().ToStdString());
            message.append(" ");
            message.append(m_config[u"smtp"][u"greeting"].AsString().ToStdString());
            m_state = Types::State::Greeted;

            // 输出日志
            LogInfo(
                u"SMTP connect: client={} -> server={}, hostname={}, greeting={}",
                GetRemoteAddress().ToString(), GetLocalAddress().ToString(),
                m_config[u"smtp"][u"hostname"].AsString(), m_config[u"smtp"][u"greeting"].AsString()
            );
            Reply(Types::ReplyCode::ServiceReady, message);
        }

        void Session::OnBytes(LikesProgram::Net::Buffer& in) {
            for (;;) {
                if (ShouldClose()) return;
                auto self = weak_from_this().lock();
                if (!self) return;
                if (m_state == Types::State::DataReceiving || m_state == Types::State::BdatReceiving) {
                    // 查找 数据 处理器
                    auto& handler = m_handlers[static_cast<size_t>(
                        m_state == Types::State::DataReceiving ? Types::CommandVerb::DATA : Types::CommandVerb::BDAT
                    )];

                    if (!handler) {
                        LogWarn(
                            u"SMTP data handler missing (verb={})",
                            std::string(m_state == Types::State::DataReceiving ? "DATA" : "BDAT")
                        );
                        Reply(Types::ReplyCode::NotImplemented, "Command not implemented");
                        return;
                    }

                    auto result = handler->InputData(self, in); // 接受 邮件数据
                    switch (result) {
                        case CommandHandler::DataResult::NeedMoreData: return; // 继续等待数据
                        case CommandHandler::DataResult::ChunkFinished: // BDAT chunk 完成，但事务未结束
                            SetState(Types::State::RcptCollecting); break;
                        case CommandHandler::DataResult::TransactionFinished: {
                            m_tx.Reset(false); // 保留 Ehlo Domain
                            SetState(Types::State::Identified); break;
                        }
                    }
                }

                // 尝试解析指令
                auto cmd = TryParseCommand(in);      // optional<Command>
                if (!cmd) break;                     // 没有完整命令行

                // 派遣指令处理
                Dispatch(*cmd);

                // 已关闭
                if (m_shouldClose) break;
            }
        }

        bool Session::ShouldClose(bool value, bool set) noexcept {
            if (set) m_shouldClose.store(value, std::memory_order_release);
            return m_shouldClose.load(std::memory_order_acquire);
        }

        void Session::Reply(Types::ReplyCode code, const std::string& message, const std::string& hyphen) {
            if (ShouldClose()) return;
            std::string messageOut = std::to_string(static_cast<int>(code)) + hyphen + message + "\r\n";
            if (m_connection) {
                m_connection->Send(messageOut.c_str(), messageOut.size());
                return;
            }

            LogError(
                u"SMTP reply failed: connection is null (code={} message='{}' state={} ehloDomain={})",
                std::to_string(static_cast<int>(code)), message, std::to_string(static_cast<int>(m_state)),
                m_tx.HasEhloDomain() ? std::string(m_tx.EhloDomain()) : std::string("unknown")
            );
        }

        void Session::SetState(Types::State state) {
            m_state = state;
        }

        Types::State Session::GetState() {
            return m_state;
        }

        void Session::StartTls() {
            if (!m_connection) return;
        
            m_connection->StartTlsAfterWriteComplete(LikeHub::SecurityContextManager::Instance().GetServerContext("lby.email"));
        
            m_isTLS = true;
        
            // RFC3207:
            // TLS 建立后必须重置 SMTP 会话状态
            m_tx.Reset(true);
        
            SetState(Types::State::Connected);
        }

        bool Session::IsTLS() {
            return m_isTLS;
        }

        Transaction& Session::GetTransaction() {
            return m_tx;
        }

        Types::Capabilities Session::GetCapabilities() {
            return m_caps;
        }

        bool Session::RelayPolicy() const noexcept {
            return m_relayPolicy;
        }

        void Session::SetUser(const Entity::User& user) {
            if (user.Valid()) m_authUser = user;
            else m_authUser.reset();
        }

        bool Session::Authenticated() const noexcept {
            return m_authUser.has_value();
        }
        const Entity::User* Session::GetUser() const noexcept {
            return Authenticated() ? &*m_authUser : nullptr;
        }

        std::string Session::SmtpHostname() const noexcept {
            return m_smtpHostname;
        }

        const LikesProgram::Net::Address& Session::GetRemoteAddress() const noexcept {
            return m_remoteAddr;
        }
        
        const LikesProgram::Net::Address& Session::GetLocalAddress() const noexcept {
            return m_localAddr;
        }
        
        std::optional<Command> Session::TryParseCommand(LikesProgram::Net::Buffer& in) {
            Command outCommand(in); // 若是有一行则会自动消费，没有则不做任何处理
            if (outCommand.NoComplete()) return std::nullopt;
            return outCommand;
        }

        void Session::Dispatch(const Command& command) {
            auto self = weak_from_this().lock();
            if (!self) return;

            if (m_state == Types::State::Authenticated) { // 正在认证，直接将指令派发给 AuthCommand
                auto& handler = m_handlers[static_cast<size_t>(Types::CommandVerb::AUTH)];
                if (!handler) {
                    LogError(
                        u"SMTP AUTH handler missing while authentication is in progress, command line: {}",
                        std::string(command.RawLine())
                    );
                    Reply(Types::ReplyCode::AuthServiceUnavailable, "Authentication service unavailable");
                    return;
                }
                auto* authHandler = dynamic_cast<AuthCommand*>(handler.get());
                if (!authHandler) {
                    LogError(
                        u"SMTP AUTH handler type mismatch while authentication is in progress, command line: {}", std::string(command.RawLine())
                    );
                    Reply(Types::ReplyCode::AuthServiceUnavailable, "Authentication service unavailable");
                    return;
                }
                authHandler->HandleAuthLine(self, std::string(command.RawLine()));
                return;
            }

            Types::CommandVerb verb = command.Verb(); // 获取指令动词
            // 不认识该指令
            if (verb == Types::CommandVerb::COUNT || verb == Types::CommandVerb::Unknown) {
                LogWarn(u"Unknown SMTP command received: '{}'", std::string(command.RawLine()));
                Reply(Types::ReplyCode::SyntaxError, "Syntax error, command unrecognized");
                return;
            }

            auto& handler = m_handlers[static_cast<size_t>(verb)];

            // 服务器认识，但未实现 (没有处理类)
            if (!handler) {
                LogWarn(u"SMTP command not implemented: '{}'", std::string(command.RawLine()));
                Reply(Types::ReplyCode::NotImplemented, "Command not implemented");
                return;
            }

            // 执行处理
            handler->Handle(self, command);
        }
    }
}