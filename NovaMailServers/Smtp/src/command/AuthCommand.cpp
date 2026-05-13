#include <command/AuthCommand.hpp>
#include <Session.hpp>
#include <address/EmailAddress.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace NovaMail {
    namespace Smtp {
        struct AuthArgs {
            std::string mechanism;
            std::optional<std::string> initialResponse;
        };

        static void SkipAuthSpaces(const std::string& text, size_t& pos) {
            while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t')) {
                ++pos;
            }
        }

        static std::string ReadAuthToken(const std::string& text, size_t& pos) {
            SkipAuthSpaces(text, pos);
        
            const size_t begin = pos;
        
            while (pos < text.size() &&
                   text[pos] != ' ' &&
                   text[pos] != '\t' &&
                   text[pos] != '\r' &&
                   text[pos] != '\n') {
                ++pos;
            }
        
            return text.substr(begin, pos - begin);
        }

        static std::string ToUpperAsciiLocal(std::string value) {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char ch) {
                    return static_cast<char>(std::toupper(ch));
                }
            );
        
            return value;
        }

        static std::string TrimAuthLine(std::string line) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
            return line;
        }

        static bool ParseAuthArgsFromRawLine(std::string_view rawLineView, AuthArgs& out) {
            out = AuthArgs{};
        
            std::string line = TrimAuthLine(std::string(rawLineView));
        
            size_t pos = 0;
        
            const std::string verb = ReadAuthToken(line, pos);
        
            if (ToUpperAsciiLocal(verb) != "AUTH") {
                return false;
            }
        
            out.mechanism = ReadAuthToken(line, pos);
        
            if (out.mechanism.empty()) {
                return false;
            }
        
            SkipAuthSpaces(line, pos);
        
            if (pos < line.size()) {
                out.initialResponse = line.substr(pos);
            }
        
            return true;
        }

        static void ReplyAuthSyntaxError(const std::shared_ptr<Session>& session) {
            if (!session) return;
            session->Reply(Types::ReplyCode::ParamError, "Syntax: AUTH mechanism [initial-response]");
            session->SetState(Types::State::Identified);
        }

        static void ReplyAuthFailed(const std::shared_ptr<Session>& session) {
            if (!session) return;
            session->Reply(Types::ReplyCode::AuthFailed, "Authentication credentials invalid");
            session->SetState(Types::State::Identified);
        }

        static void ReplyAuthSucceeded(const std::shared_ptr<Session>& session) {
            if (!session) return;
            session->Reply(Types::ReplyCode::AuthSucceeded, "Authentication successful");
            session->SetState(Types::State::Identified);
        }

        static void ReplyAuthContinue(const std::shared_ptr<Session>& session, const std::string& challenge) {
            if (!session) return;
            session->Reply(Types::ReplyCode::AuthContinue, challenge);
        }

        void AuthCommand::ResetAuthState(const std::shared_ptr<Session>& session) {
            m_stage = Stage::None;
            m_pendingAuthUser.clear();
            session->SetState(Types::State::Identified);
        }

        void AuthCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;
            const std::string rawLine = std::string(command.RawLine());

            Types::State state = session->GetState();
            // AUTH 必须在 EHLO/HELO 之后
            if (state == Types::State::Connected || state == Types::State::Greeted) {
                LogWarn(u"AUTH command before EHLO/HELO, command line: {}", rawLine);
                session->Reply(Types::ReplyCode::BadSequence, "Bad sequence of commands, send HELO/EHLO first");
                return;
            }

            // 当前对话非 TLS
            if (!session->IsTLS()) {
                LogWarn(
                    u"AUTH rejected before STARTTLS, command line: {}, client={} -> server={}",
                    rawLine, session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                );
                session->Reply(Types::ReplyCode::AuthenticationRequired, "Must issue a STARTTLS command first");
                return;
            }

            // 已经完成登录认证
            if (session->Authenticated()) {
                const Entity::User* user = session->GetUser();
                LogWarn(
                    u"AUTH command rejected: already authenticated, userId={}, command line: {}, client={} -> server={}",
                    user ? std::to_string(user->userId) : "unknown", rawLine, session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                );
                session->Reply(Types::ReplyCode::BadSequence, "Already authenticated");
                return;
            }

            AuthArgs authArgs;

            if (!ParseAuthArgsFromRawLine(rawLine, authArgs) || authArgs.mechanism.empty()) {
                LogWarn(u"AUTH command missing mechanism, command line: {}", rawLine);
                ReplyAuthSyntaxError(session);
                return;
            }

            std::string mechanism = ToUpper(authArgs.mechanism);

            if (mechanism == "LOGIN") {
                session->SetState(Types::State::Authenticated);
                HandleLogin(session, authArgs.initialResponse);
                return;
            }

            if (mechanism == "PLAIN") {
                session->SetState(Types::State::Authenticated);
                HandlePlain(session, authArgs.initialResponse);
                return;
            }

            LogWarn(
                u"SMTP AUTH unsupported mechanism: mechanism={}, command line: {}",
                LikesProgram::String(mechanism), rawLine
            );
            session->Reply(Types::ReplyCode::ParamUnsupported, "Unsupported authentication mechanism");
        }

        void AuthCommand::HandleLogin(const std::shared_ptr<Session>& session, const std::optional<std::string>& initialResponse) {
            /*
             * AUTH LOGIN
             *
             * C: AUTH LOGIN
             * S: 334 VXNlcm5hbWU6
             * C: base64(username)
             * S: 334 UGFzc3dvcmQ6
             * C: base64(password)
             * S: 235 2.7.0 Authentication successful
             *
             * 也支持：
             *
             * C: AUTH LOGIN base64(username)
             * S: 334 UGFzc3dvcmQ6
             * C: base64(password)
             */

            if (initialResponse && !initialResponse->empty() && *initialResponse != "=") {
                auto decodedUser = DecodeBase64(*initialResponse);

                if (!decodedUser || decodedUser->empty()) {
                    LogWarn(
                        u"SMTP AUTH LOGIN rejected: invalid initial username response, client={} -> server={}",
                        session->GetRemoteAddress().ToString(),
                        session->GetLocalAddress().ToString()
                    );

                    ResetAuthState(session);
                    ReplyAuthFailed(session);
                    return;
                }

                m_pendingAuthUser = *decodedUser;
                m_stage = Stage::LoginPassword;

                // Base64("Password:") = UGFzc3dvcmQ6
                ReplyAuthContinue(session, "UGFzc3dvcmQ6");
                return;
            }

            m_stage = Stage::LoginUsername;

            // Base64("Username:") = VXNlcm5hbWU6
            ReplyAuthContinue(session, "VXNlcm5hbWU6");
        }

        void AuthCommand::HandlePlain(const std::shared_ptr<Session>& session, const std::optional<std::string>& initialResponse) {
            /*
             * AUTH PLAIN base64("\0username\0password")
             *
             * 也支持：
             *
             * C: AUTH PLAIN
             * S: 334
             * C: base64("\0username\0password")
             */
            if (!initialResponse) {
                m_stage = Stage::PlainResponse;

                // 空 challenge
                ReplyAuthContinue(session, "");
                return;
            }

            HandlePlainResponse(session, *initialResponse);
        }

        void AuthCommand::HandleAuthLine(const std::shared_ptr<Session>& session, const std::string& rawLine) {
            if (!session) return;

            std::string line = TrimAuthLine(rawLine);

            // RFC 4954：客户端发送 "*" 表示取消认证
            if (line == "*") {
                LogWarn(
                    u"SMTP AUTH canceled by client: client={} -> server={}",
                    session->GetRemoteAddress().ToString(),
                    session->GetLocalAddress().ToString()
                );
                ResetAuthState(session);
                session->Reply(Types::ReplyCode::ParamError, "Authentication canceled");
                return;
            }

            switch (m_stage) {
            case Stage::LoginUsername:
                HandleLoginUsername(session, line);
                return;

            case Stage::LoginPassword:
                HandleLoginPassword(session, line);
                return;

            case Stage::PlainResponse:
                HandlePlainResponse(session, line);
                return;

            case Stage::None:
            default:
                LogWarn(
                    u"SMTP AUTH line rejected: no auth stage, line={}, client={} -> server={}",
                    LikesProgram::String(line),
                    session->GetRemoteAddress().ToString(),
                    session->GetLocalAddress().ToString()
                );
                session->Reply(Types::ReplyCode::BadSequence, "Bad sequence of commands");
                return;
            }
        }

        void AuthCommand::HandleLoginUsername(const std::shared_ptr<Session>& session, const std::string& line) {
            auto decodedUser = DecodeBase64(line);

            if (!decodedUser || decodedUser->empty()) {
                LogWarn(
                    u"SMTP AUTH LOGIN rejected: invalid username response, client={} -> server={}",
                    session->GetRemoteAddress().ToString(),
                    session->GetLocalAddress().ToString()
                );

                ResetAuthState(session);
                ReplyAuthFailed(session);
                return;
            }

            m_pendingAuthUser = *decodedUser;
            m_stage = Stage::LoginPassword;

            // Base64("Password:")
            ReplyAuthContinue(session, "UGFzc3dvcmQ6");
        }

        void AuthCommand::HandleLoginPassword(const std::shared_ptr<Session>& session, const std::string& line) {
            auto decodedPassword = DecodeBase64(line);

            if (!decodedPassword) {
                LogWarn(
                    u"SMTP AUTH LOGIN rejected: invalid password response, account={}, client={} -> server={}",
                    LikesProgram::String(m_pendingAuthUser),
                    session->GetRemoteAddress().ToString(),
                    session->GetLocalAddress().ToString()
                );

                ResetAuthState(session);
                ReplyAuthFailed(session);
                return;
            }

            auto user = VerifyCredential(session, "LOGIN", m_pendingAuthUser, *decodedPassword);

            if (!user) {
                ResetAuthState(session);
                ReplyAuthFailed(session);
                return;
            }

            session->SetUser(*user);
            ResetAuthState(session);

            LogInfo(
                u"SMTP AUTH LOGIN success: account={}, userId={}, client={} -> server={}",
                LikesProgram::String(user->account),
                std::to_string(user->userId),
                session->GetRemoteAddress().ToString(),
                session->GetLocalAddress().ToString()
            );

            ReplyAuthSucceeded(session);
        }

        void AuthCommand::HandlePlainResponse(const std::shared_ptr<Session>& session, const std::string& line) {
            const std::string base64Text = (line == "=") ? std::string{} : line;

            auto decoded = DecodeBase64(base64Text);

            if (!decoded) {
                LogWarn(
                    u"SMTP AUTH PLAIN rejected: invalid base64 response, client={} -> server={}",
                    session->GetRemoteAddress().ToString(),
                    session->GetLocalAddress().ToString()
                );

                ResetAuthState(session);
                ReplyAuthFailed(session);
                return;
            }

            auto parts = SplitPlainAuth(*decoded);

            /*
             * AUTH PLAIN 格式：
             *
             * authorization-id \0 authentication-id \0 password
             *
             * 通常 authorization-id 为空，所以 parts[0] 可以为空；
             * parts[1] 是账号；
             * parts[2] 是密码/授权码。
             */
            if (parts.size() < 3 || parts[1].empty()) {
                LogWarn(
                    u"SMTP AUTH PLAIN rejected: invalid plain auth format, partCount={}, client={} -> server={}",
                    std::to_string(parts.size()),
                    session->GetRemoteAddress().ToString(),
                    session->GetLocalAddress().ToString()
                );

                ResetAuthState(session);
                ReplyAuthFailed(session);
                return;
            }

            const std::string& account = parts[1];
            const std::string& password = parts[2];

            auto user = VerifyCredential(session, "PLAIN", account, password);

            if (!user) {
                ResetAuthState(session);
                ReplyAuthFailed(session);
                return;
            }

            session->SetUser(*user);
            ResetAuthState(session);

            LogInfo(
                u"SMTP AUTH PLAIN success: account={}, userId={}, client={} -> server={}",
                LikesProgram::String(user->account),
                std::to_string(user->userId),
                session->GetRemoteAddress().ToString(),
                session->GetLocalAddress().ToString()
            );

            ReplyAuthSucceeded(session);
        }

        std::optional<Entity::User> AuthCommand::VerifyCredential(const std::shared_ptr<Session>& session, std::string_view mechanism, const std::string& account, const std::string& password) {
            if (!session) return std::nullopt;

            if (account.empty() || password.empty()) {
                LogWarn(
                    u"SMTP AUTH rejected: empty account or password, mechanism={}, client={} -> server={}",
                    std::string(mechanism),
                    session->GetRemoteAddress().ToString(),
                    session->GetLocalAddress().ToString()
                );

                return std::nullopt;
            }

            Entity::User user;
            bool success = false;
            // 邮箱登录
            Address::EmailAddress address;
            if (Address::EmailAddress::Parse(account, address))
                success = m_userService.GetUserByMailboxAuthCode(address.Local(), address.GetDomain().Name(), password, user);
            // 普通账号登录
            else success = m_userService.GetUserByAccountAuthCode(account, password, user);
            
            if (!success || !user.Valid()) return std::nullopt;
            
            return user;
        }

        std::string AuthCommand::ToUpper(std::string value) {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char ch) {
                    return static_cast<char>(std::toupper(ch));
                }
            );

            return value;
        }

        std::optional<std::string> AuthCommand::DecodeBase64(std::string_view input) {
            static constexpr std::array<int, 256> table = [] {
                std::array<int, 256> t{};
                t.fill(-1);

                for (int i = 'A'; i <= 'Z'; ++i) {
                    t[static_cast<size_t>(i)] = i - 'A';
                }

                for (int i = 'a'; i <= 'z'; ++i) {
                    t[static_cast<size_t>(i)] = i - 'a' + 26;
                }

                for (int i = '0'; i <= '9'; ++i) {
                    t[static_cast<size_t>(i)] = i - '0' + 52;
                }

                t[static_cast<size_t>('+')] = 62;
                t[static_cast<size_t>('/')] = 63;

                return t;
            }();

            std::string output;
            int val = 0;
            int valb = -8;

            for (unsigned char ch : input) {
                if (std::isspace(ch)) {
                    continue;
                }

                if (ch == '=') {
                    break;
                }

                int decoded = table[static_cast<size_t>(ch)];

                if (decoded < 0) {
                    return std::nullopt;
                }

                val = (val << 6) + decoded;
                valb += 6;

                if (valb >= 0) {
                    output.push_back(static_cast<char>((val >> valb) & 0xFF));
                    valb -= 8;
                }
            }

            return output;
        }

        std::vector<std::string> AuthCommand::SplitPlainAuth(const std::string& decoded) {
            std::vector<std::string> result;
            std::string current;

            for (char ch : decoded) {
                if (ch == '\0') {
                    result.push_back(current);
                    current.clear();
                } else {
                    current.push_back(ch);
                }
            }

            result.push_back(current);

            return result;
        }
    }
}