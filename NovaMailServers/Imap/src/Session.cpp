#include <Session.hpp>
#include <Connection.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <LikeHub/SecurityContextManager.hpp>

#include <command/CapabilityCommand.hpp>
#include <command/NoopCommand.hpp>
#include <command/LogoutCommand.hpp>

#include <command/StartTlsCommand.hpp>
#include <command/AuthenticateCommand.hpp>
#include <command/LoginCommand.hpp>

#include <command/SelectCommand.hpp>

#include <command/ListCommand.hpp>
#include <command/StatusCommand.hpp>
#include <command/AppendCommand.hpp>

#include <command/CheckCommand.hpp>
#include <command/CloseCommand.hpp>
#include <command/FetchCommand.hpp>
#include <command/StoreCommand.hpp>

namespace NovaMail {
    namespace Imap {
        const char* StatusToString(Status status) noexcept {
            switch (status) {
                case Status::Ok: return "OK";
                case Status::No: return "NO";
                case Status::Bad: return "BAD";
                default: return "BAD";
            }
        }

        Session::Session(Connection* connection, LikesProgram::Net::Address remoteAddr, LikesProgram::Net::Address localAddr) : m_connection(connection), m_remoteAddr(std::move(remoteAddr)), m_localAddr(std::move(localAddr)) {
            try {
                LikeHub::LikeHubConfig& config = LikeHub::LikeHubConfig::Instance();
                m_config = config.Raw();

                auto hostname = m_config[u"imap"][u"hostname"].AsString().ToStdString();
                if (!hostname.empty()) m_imapHostname = std::move(hostname);
                m_caps = Capability::IMAP4rev2;
                if (m_config[u"imap"][u"start_tls"].AsBool()) m_caps |= Capability::STARTTLS;
                m_caps |= Capability::AUTH;
                m_caps |= Capability::AUTH_PLAIN;
                m_caps |= Capability::AUTH_LOGIN;
                m_caps |= Capability::LITERAL_PLUS;

                // 后续真正实现后再打开
                // m_caps |= Capability::IDLE;
                // m_caps |= Capability::MOVE;
                // m_caps |= Capability::NAMESPACE;
                // m_caps |= Capability::UNSELECT;
                // m_caps |= Capability::ENABLE;
            } catch (const std::exception& e) {
                LogWarn(u"IMAP session config load failed, use default hostname: err={}", e.what());
            }

            InitHandlers();

            LogInfo(
                u"IMAP session created: client={} -> server={}, hostname={}",
                GetRemoteAddress().ToString(), GetLocalAddress().ToString(), LikesProgram::String(m_imapHostname)
            );
        }
        
        void Session::InitHandlers() {
            m_handlers[static_cast<size_t>(CommandVerb::CAPABILITY)] = std::make_unique<CapabilityCommand>();
            m_handlers[static_cast<size_t>(CommandVerb::NOOP)] = std::make_unique<NoopCommand>();
            m_handlers[static_cast<size_t>(CommandVerb::LOGOUT)] = std::make_unique<LogoutCommand>();

            m_handlers[static_cast<size_t>(CommandVerb::STARTTLS)] = std::make_unique<StartTlsCommand>();
            m_handlers[static_cast<size_t>(CommandVerb::AUTHENTICATE)] = std::make_unique<AuthenticateCommand>();
            m_handlers[static_cast<size_t>(CommandVerb::LOGIN)] = std::make_unique<LoginCommand>();

            m_handlers[static_cast<size_t>(CommandVerb::SELECT)] = std::make_unique<SelectCommand>(false);
            m_handlers[static_cast<size_t>(CommandVerb::EXAMINE)] = std::make_unique<SelectCommand>(true);

            m_handlers[static_cast<size_t>(CommandVerb::LIST)] = std::make_unique<ListCommand>(Mode::List);
            m_handlers[static_cast<size_t>(CommandVerb::LSUB)] = std::make_unique<ListCommand>(Mode::Lsub);
            m_handlers[static_cast<size_t>(CommandVerb::STATUS)] = std::make_unique<StatusCommand>();
            m_handlers[static_cast<size_t>(CommandVerb::APPEND)] = std::make_unique<AppendCommand>();

            m_handlers[static_cast<size_t>(CommandVerb::CHECK)] = std::make_unique<CheckCommand>();
            m_handlers[static_cast<size_t>(CommandVerb::CLOSE)] = std::make_unique<CloseCommand>();
            m_handlers[static_cast<size_t>(CommandVerb::FETCH)] = std::make_unique<FetchCommand>();
            m_handlers[static_cast<size_t>(CommandVerb::STORE)] = std::make_unique<StoreCommand>();

            /*
             * 这里先不强行写具体 handler，避免你当前类还没建好时编译不过。
             *
             * 后续建议这样逐个注册：
             *
             * m_handlers[static_cast<size_t>(CommandVerb::LOGIN)] =
             *     std::make_unique<LoginCommand>();
             *
             * m_handlers[static_cast<size_t>(CommandVerb::SELECT)] =
             *     std::make_unique<SelectCommand>();
             *
             * m_handlers[static_cast<size_t>(CommandVerb::FETCH)] =
             *     std::make_unique<FetchCommand>();
             *
             * m_handlers[static_cast<size_t>(CommandVerb::STORE)] =
             *     std::make_unique<StoreCommand>();
             *
             * 注意：
             * UID FETCH / UID STORE 已经在 Command 解析层归一化为 FETCH / STORE，
             * handler 内部通过 command.UidMode() 判断是否按 UID 查询。
             */
        }

        void Session::OnConnect() {
            if (ShouldClose()) {
                return;
            }

            m_state = State::NotAuthenticated;

            ReplyUntagged("OK " + m_imapHostname + " IMAP Service Ready");

            LogInfo(
                u"IMAP connect: client={} -> server={}, hostname={}",
                GetRemoteAddress().ToString(), GetLocalAddress().ToString(), LikesProgram::String(m_imapHostname)
            );
        }

        void Session::OnBytes(LikesProgram::Net::Buffer& in) {
            for (;;) {
                if (ShouldClose()) return;
                auto self = weak_from_this().lock();
                if (!self) {
                    LogDebug(u"IMAP OnBytes exit: shouldClose=true");
                    ShouldClose(true, true);
                    return;
                }

                // 原始字节输入
                if (m_inputMode == InputMode::LiteralBytes) {
                    LogDebug(u"IMAP input mode: LiteralBytes");
                    const size_t index = static_cast<size_t>(m_activeVerb);
                    if (index >= m_handlers.size() || !m_handlers[index]) {
                        ReplyTagged(m_activeTag.empty() ? "*" : m_activeTag, Status::Bad, "Literal command handler missing");
                        FinishActiveCommand();
                        return;
                    }
                
                    auto result = m_handlers[index]->InputData(self, in);
                    switch (result) {
                        case InputResult::NeedMoreData: return;
                        case InputResult::Continue: continue;
                        case InputResult::Finished:
                            FinishActiveCommand();
                            continue;
                        case InputResult::Failed:
                        default:
                            ReplyTagged(m_activeTag.empty() ? "*" : m_activeTag, Status::Bad, "Literal command failed");
                            FinishActiveCommand();
                            continue;
                    }
                }

                // 普通指令 或 行级 模式先读取一整行
                auto command = TryParseCommand(in);
                if (!command.has_value()) return;

                // 行级模式
                if (m_inputMode == InputMode::ActiveLine) {
                    LogDebug(u"IMAP input mode: ActiveLine");
                    DispatchActiveLine(*command);
                    continue;
                }

                LogDebug(u"IMAP input mode: CommandLine");
                // 普通指令模式
                Dispatch(*command);

                if (ShouldClose()) return;
            }
        }

        void Session::OnTlsHandshakeDone() {
            LogInfo(
                u"IMAP TLS handshake done: client={} -> server={}", GetRemoteAddress().ToString(), GetLocalAddress().ToString()
            );

            // TLS 前的认证状态不能继承
            m_state = State::NotAuthenticated;
            m_authUser.reset();
            FinishActiveCommand(); // 清空正在处理的指令
        }

        bool Session::ShouldClose(bool value, bool set) noexcept {
            if (set) m_shouldClose.store(value, std::memory_order_release);
            return m_shouldClose.load(std::memory_order_acquire);
        }

        void Session::ReplyTagged(const std::string& tag, Status status, const std::string& message) {
            if (ShouldClose()) return;

            std::string out;

            const std::string safeTag = tag.empty() ? "*" : tag;

            out.reserve(
                safeTag.size()
                + message.size()
                + 16
            );

            out.append(safeTag);
            out.push_back(' ');
            out.append(StatusToString(status));

            if (!message.empty()) {
                out.push_back(' ');
                out.append(message);
            }

            out.append("\r\n");

            SendRaw(out);
        }
        void Session::ReplyUntagged(const std::string& message) {
            if (ShouldClose()) return;

            std::string out;
            out.reserve(message.size() + 4);

            out.append("*");

            if (!message.empty()) {
                out.push_back(' ');
                out.append(message);
            }

            out.append("\r\n");

            SendRaw(out);

        }
        void Session::ReplyContinuation(const std::string& message) {
            if (ShouldClose()) return;

            std::string out;
            out.reserve(message.size() + 4);

            out.append("+");

            if (!message.empty()) {
                out.push_back(' ');
                out.append(message);
            }

            out.append("\r\n");

            SendRaw(out);

        }

        void Session::SetState(State state) {
            m_state = state;
        }
        
        State Session::GetState() const noexcept {
            return m_state;
        }

        void Session::StartTls() {
            if (!m_connection) return;
            m_connection->StartTlsAfterWriteComplete(LikeHub::SecurityContextManager::Instance().GetServerContext("lby.email"));
            m_isTLS = true;
        }

        bool Session::IsTLS() {
            return m_isTLS;
        }

        void Session::SetUser(const Entity::User& user) {
            if (user.Valid()) {
                m_authUser = user;
                SetState(State::Authenticated);
            } else  {
                m_authUser.reset();
                m_state = State::NotAuthenticated;
            }
        }

        bool Session::Authenticated() const noexcept {
            return m_authUser.has_value() && m_authUser->mailBoxId != 0;
        }
        const Entity::User* Session::GetUser() const noexcept {
            return Authenticated() ? &*m_authUser : nullptr;
        }

        Capability Session::GetCapabilities() const noexcept {
            return m_caps;
        }

        void Session::SelectMailbox(const Service::MailFolderInfo& mailbox, bool readOnly, uint64_t exists) {
            if (!mailbox.Valid()) return;

            m_selectedMailbox = mailbox;
            m_selectedReadOnly = readOnly;
            m_selectedExists = exists;
            m_state = State::Selected;
        }

        uint64_t Session::GetSelectedExists() const noexcept {
            return m_selectedExists;
        }

        void Session::UpdateSelectedExists(uint64_t exists) noexcept {
            m_selectedExists = exists;
        }

        void Session::ClearSelectedMailbox() {
            m_selectedMailbox.reset();
            m_selectedReadOnly = false;
            m_selectedExists = 0;
        
            if (Authenticated()) m_state = State::Authenticated;
            else m_state = State::NotAuthenticated;
        }

        bool Session::HasSelectedMailbox() const noexcept {
            return m_selectedMailbox.has_value();
        }

        const Service::MailFolderInfo* Session::GetSelectedMailbox() const noexcept {
            return m_selectedMailbox ? &*m_selectedMailbox : nullptr;
        }

        bool Session::SelectedReadOnly() const noexcept {
            return m_selectedReadOnly;
        }

        std::string Session::ImapHostname() const noexcept {
            return m_imapHostname;
        }

        void Session::BeginActiveLine(CommandVerb verb, std::string tag) {
            m_activeVerb = verb;
            m_activeTag = std::move(tag);
            m_inputMode = InputMode::ActiveLine;
        }
        
        void Session::BeginLiteralBytes(CommandVerb verb, std::string tag) {
            m_activeVerb = verb;
            m_activeTag = std::move(tag);
            m_inputMode = InputMode::LiteralBytes;
        }

        std::string_view Session::ActiveTag() const noexcept {
            return m_activeTag;
        }
        
        void Session::FinishActiveCommand() {
            m_activeVerb = CommandVerb::Unknown;
            m_activeTag.clear();
            m_inputMode = InputMode::CommandLine;
        }

        const LikesProgram::Net::Address& Session::GetRemoteAddress() const noexcept {
            return m_remoteAddr;
        }
        
        const LikesProgram::Net::Address& Session::GetLocalAddress() const noexcept {
            return m_localAddr;
        }

        std::optional<Command> Session::TryParseCommand(LikesProgram::Net::Buffer& in) {
            for (;;) {
                Command outCommand(in); // 若有一行则会自动消费，没有则不做任何处理
            
                if (outCommand.NoComplete()) return std::nullopt;
            
                // 忽略空行，避免 APPEND literal 后 curl/客户端额外发送 CRLF 导致 bad syntax
                if (outCommand.RawLine().empty()) {
                    LogDebug(u"IMAP empty command line ignored");
                    continue;
                }
            
                return outCommand;
            }
        }

        void Session::DispatchActiveLine(const Command& line) {
            auto self = weak_from_this().lock();
        
            if (!self) {
                LogWarn(
                    u"IMAP dispatch aborted: session expired, rawLine='{}'",
                    std::string(line.RawLine())
                );
                ShouldClose(true, true);
                return;
            }
        
            const size_t index = static_cast<size_t>(m_activeVerb);
        
            if (index >= m_handlers.size()) {
                ReplyTagged(m_activeTag.empty() ? "*" : m_activeTag, Status::Bad, "Invalid active command");
                FinishActiveCommand();
                return;
            }
        
            auto& handler = m_handlers[index];
        
            if (!handler) {
                ReplyTagged(m_activeTag.empty() ? "*" : m_activeTag, Status::Bad, "Active command handler missing");
                FinishActiveCommand();
                return;
            }
        
            const auto result = handler->InputLine(self, line);
        
            switch (result) {
                case InputResult::NeedMoreData: return;
                case InputResult::Continue: return;
                case InputResult::Finished:
                    FinishActiveCommand();
                    return;
                case InputResult::Failed:
                default:
                    ReplyTagged(m_activeTag.empty() ? "*" : m_activeTag, Status::Bad, "Active command failed");
                    FinishActiveCommand();
                    return;
            }
        }

        void Session::Dispatch(const Command& command) {
            auto self = weak_from_this().lock();
            if (!self) {
                LogWarn(
                    u"IMAP dispatch aborted: session expired, rawLine='{}'",
                    std::string(std::string(command.RawLine()))
                );
                ShouldClose(true, true);
                return;
            }
        
            const auto verb = command.Verb();
            const std::string tag = std::string(command.Tag());
        
            if (verb == CommandVerb::Unknown || verb == CommandVerb::COUNT) {
                LogWarn(u"IMAP dispatch bad syntax: rawLine='{}' verb={}", std::string(command.RawLine()), static_cast<int64_t>(verb));
                ReplyTagged(tag.empty() ? "*" : tag, Status::Bad, "Bad command syntax");
                return;
            }
        
            const size_t index = static_cast<size_t>(verb);
            if (index >= m_handlers.size()) {
                LogWarn(u"IMAP dispatch invalid command index: rawLine='{}' verb={}", std::string(command.RawLine()), static_cast<int64_t>(verb));
                ReplyTagged(tag.empty() ? "*" : tag, Status::Bad, "Invalid command");
                return;
            }
        
            auto& handler = m_handlers[index];
        
            if (!handler) {
                LogWarn(u"IMAP dispatch handler missing: rawLine='{}' verb={}", std::string(command.RawLine()), static_cast<int64_t>(verb));
                ReplyTagged(tag.empty() ? "*" : tag, Status::Bad, "Command not implemented");
                return;
            }
        
            if (!handler->IsAllowed(*this, command)) {
                LogWarn(u"IMAP dispatch command not allowed: rawLine='{}' verb={}", std::string(command.RawLine()), static_cast<int64_t>(verb));
                ReplyTagged(tag.empty() ? "*" : tag, Status::Bad, "Command not allowed in current state");
                return;
            }

            handler->Handle(self, command);
        }

        void Session::SendRaw(const std::string& data) {
            if (data.empty()) return;
            if (ShouldClose()) return;
            if (!m_connection) {
                LogError(u"IMAP send failed: connection is null, dataSize={}", static_cast<int64_t>(data.size()));
                return;
            }
            m_connection->Send(data.data(), data.size());
        }
    }
}