#pragma once
#include <command/CommandHandler.hpp>
#include <service/UserService.hpp>
#include <string>

namespace NovaMail {
    namespace Imap {
        class AuthenticateCommand final : public NotAuthenticatedCommandHandler {
        public:
            AuthenticateCommand(): m_userService(LikeHub::Storage::DatabasePool::Instance()) { }
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;
            InputResult InputLine(const std::shared_ptr<Session>& session, const Command& line) override;
        private:
            enum class Mechanism {
                None,
                Plain,
                Login
            };
            enum class LoginStage {
                None,
                WaitUsername,
                WaitPassword
            };
            bool HandlePlainInitial(const std::shared_ptr<Session>& session, const std::string& tag, const std::string& base64Text);
            bool HandleLoginUsername(const std::shared_ptr<Session>& session, const std::string& base64Text);
            bool HandleLoginPassword(const std::shared_ptr<Session>& session, const std::string& tag, const std::string& base64Text);
            bool VerifyAndLogin(const std::shared_ptr<Session>& session, const std::string& tag, const std::string& username, const std::string& password);
        private:
            Mechanism m_mechanism = Mechanism::None;
            LoginStage m_loginStage = LoginStage::None;
            std::string m_loginUsername;
            Service::UserService m_userService;
        };

    }
}