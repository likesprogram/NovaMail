#include <command/AuthenticateCommand.hpp>
#include <Session.hpp>
#include <command/Command.hpp>
#include <address/EmailAddress.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace NovaMail {
    namespace Imap {
        static std::string ToUpperAscii(std::string text) {
            std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
            return text;
        }

        std::vector<std::string> SplitSpaces(const std::string& text) {
            std::istringstream iss(text);
            std::vector<std::string> out;
            std::string item;
            while (iss >> item)  out.push_back(item);
            return out;
        }

        static bool Base64Decode(const std::string& input, std::string& output) {
            output.clear();
            if (input.empty()) return false;

            std::vector<unsigned char> buffer((input.size() * 3) / 4 + 4);
            int len = EVP_DecodeBlock(buffer.data(), reinterpret_cast<const unsigned char*>(input.data()), static_cast<int>(input.size()));
            if (len < 0) return false;

            size_t padding = 0;
            if (!input.empty() && input.back() == '=') ++padding;
            if (input.size() >= 2 && input[input.size() - 2] == '=') ++padding;
            if (static_cast<size_t>(len) < padding) return false;
            output.assign(reinterpret_cast<const char*>(buffer.data()), static_cast<size_t>(len) - padding);

            return true;
        }

        static bool ParsePlainDecoded(const std::string& decoded, std::string& account, std::string& password) {
            const size_t firstNul = decoded.find('\0');
            if (firstNul == std::string::npos) return false;

            const size_t secondNul = decoded.find('\0', firstNul + 1);
            if (secondNul == std::string::npos) return false;

            account = decoded.substr(firstNul + 1, secondNul - firstNul - 1);
            password = decoded.substr(secondNul + 1);
            return !account.empty();
        }

        void AuthenticateCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;

            const std::string tag = std::string(command.Tag());
            const std::string args = std::string(command.Arguments());

            // STARTTLS 前不要允许 AUTHENTICATE
            if (!session->IsTLS()) {
                session->ReplyTagged(tag, Status::No, "Authentication requires TLS");
                return;
            }

            auto parts = SplitSpaces(args);

            if (parts.empty()) {
                session->ReplyTagged(tag, Status::Bad, "AUTHENTICATE requires mechanism");
                return;
            }

            const std::string mechanism = ToUpperAscii(parts[0]);

            if (mechanism == "PLAIN") {
                m_mechanism = Mechanism::Plain;
                m_loginStage = LoginStage::None;
                m_loginUsername.clear();

                // A001 AUTHENTICATE PLAIN
                if (parts.size() == 1) {
                    session->ReplyContinuation("");
                    session->BeginActiveLine(CommandVerb::AUTHENTICATE, tag);
                    return;
                }

                // A001 AUTHENTICATE PLAIN <initial-response>
                if (parts.size() == 2) {
                    HandlePlainInitial(session, tag, parts[1]);
                    return;
                }

                session->ReplyTagged(tag, Status::Bad, "Bad AUTHENTICATE PLAIN arguments");
                return;
            }

            if (mechanism == "LOGIN") {
                m_mechanism = Mechanism::Login;
                m_loginStage = LoginStage::WaitUsername;
                m_loginUsername.clear();

                // AUTHENTICATE LOGIN 先要求用户名
                session->ReplyContinuation("VXNlcm5hbWU6");
                session->BeginActiveLine(CommandVerb::AUTHENTICATE, tag);
                return;
            }

            session->ReplyTagged(tag, Status::No,"Unsupported authentication mechanism");
        }

        InputResult AuthenticateCommand::InputLine(const std::shared_ptr<Session>& session, const Command& line) {
            if (!session) return InputResult::Failed;

            const std::string tag = std::string(session->ActiveTag());
            const std::string rawLine = std::string(line.RawLine());

            if (m_mechanism == Mechanism::Plain) {
                HandlePlainInitial(session, tag, rawLine);
                return InputResult::Finished;
            }

            if (m_mechanism == Mechanism::Login) {
                if (m_loginStage == LoginStage::WaitUsername) {
                    if (!HandleLoginUsername(session, rawLine)) return InputResult::Finished;

                    // AUTHENTICATE LOGIN 要求密码
                    session->ReplyContinuation("UGFzc3dvcmQ6");
                    m_loginStage = LoginStage::WaitPassword;
                    return InputResult::Continue;
                }

                if (m_loginStage == LoginStage::WaitPassword) {
                    HandleLoginPassword(session, tag, rawLine);
                    return InputResult::Finished;
                }
            }

            return InputResult::Failed;
        }

        bool AuthenticateCommand::HandlePlainInitial(const std::shared_ptr<Session>& session, const std::string& tag, const std::string& base64Text) {
            std::string decoded;

            if (!Base64Decode(base64Text, decoded)) {
                session->ReplyTagged(tag, Status::Bad, "Invalid base64 authentication response");
                return false;
            }

            std::string account;
            std::string password;

            if (!ParsePlainDecoded(decoded, account, password)) {
                session->ReplyTagged(tag, Status::Bad, "Invalid PLAIN authentication response");
                return false;
            }

            return VerifyAndLogin(session, tag, account, password);
        }

        bool AuthenticateCommand::HandleLoginUsername(const std::shared_ptr<Session>& session, const std::string& base64Text) {
            std::string account;

            if (!Base64Decode(base64Text, account) || account.empty()) {
                session->ReplyTagged(std::string(session->ActiveTag()), Status::Bad, "Invalid LOGIN account");
                return false;
            }

            m_loginUsername = std::move(account);
            return true;
        }

        bool AuthenticateCommand::HandleLoginPassword(const std::shared_ptr<Session>& session, const std::string& tag, const std::string& base64Text) {
            std::string password;

            if (!Base64Decode(base64Text, password)) {
                session->ReplyTagged(tag, Status::Bad, "Invalid LOGIN password");
                return false;
            }

            return VerifyAndLogin(session, tag, m_loginUsername, password);
        }

        bool AuthenticateCommand::VerifyAndLogin(const std::shared_ptr<Session>& session, const std::string& tag, const std::string& account, const std::string& password) {
            LogInfo(
                u"IMAP AUTH attempt: user={}, client={} -> server={}",
                LikesProgram::String(account), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
            );

            Entity::User user;
            bool success = false;
            // 邮箱登录
            Address::EmailAddress address;
            if (Address::EmailAddress::Parse(account, address))
                success = m_userService.GetUserByMailboxAuthCode(address.Local(), address.GetDomain().Name(), password, user);
            // 普通账号登录
            else success = m_userService.GetUserByAccountAuthCode(account, password, user);
            
            if (!success || !user.Valid()) {
                session->ReplyTagged(tag, Status::No, "Authentication failed");
                return false;
            }

            if (!user.HasMailBox()) {
                LogWarn(
                    u"IMAP LOGIN failed: user={} userId={} reason=NoAvailableMailbox",
                    LikesProgram::String(account), static_cast<int64_t>(user.userId)
                );
            
                session->ReplyTagged(tag, Status::No, "No available mailbox");
                return false;
            }

            session->SetUser(user);
            session->ReplyTagged(tag, Status::Ok, "AUTHENTICATE completed");
            return true;
        }
    }
}