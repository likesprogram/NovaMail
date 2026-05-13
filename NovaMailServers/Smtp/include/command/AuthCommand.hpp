#pragma once
#include <command/CommandHandler.hpp>
#include <command/Command.hpp>
#include <types/Types.hpp>
#include <service/UserService.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace NovaMail {
    namespace Smtp {
        class AuthCommand final : public CommandHandler {
        public:
            AuthCommand(): m_userService(LikeHub::Storage::DatabasePool::Instance()) { }

            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;

            // AUTH LOGIN / AUTH PLAIN 后续行处理，由 Session 按行解析后调用
            void HandleAuthLine(const std::shared_ptr<Session>& session, const std::string& line);
        private:
            enum class Stage : uint8_t {
                None = 0,
                LoginUsername,
                LoginPassword,
                PlainResponse
            };

        private:
            void ResetAuthState(const std::shared_ptr<Session>& session);

            void HandleLogin(const std::shared_ptr<Session>& session, const std::optional<std::string>& initialResponse);
            void HandlePlain(const std::shared_ptr<Session>& session, const std::optional<std::string>& initialResponse);

            void HandleLoginUsername(const std::shared_ptr<Session>& session, const std::string& line);
            void HandleLoginPassword(const std::shared_ptr<Session>& session, const std::string& line);
            void HandlePlainResponse(const std::shared_ptr<Session>& session, const std::string& line);

            std::optional<Entity::User> VerifyCredential(const std::shared_ptr<Session>& session, std::string_view mechanism, const std::string& account, const std::string& password);

            static std::string ToUpper(std::string value);
            static std::optional<std::string> DecodeBase64(std::string_view input);
            static std::vector<std::string> SplitPlainAuth(const std::string& decoded);

        private:
            Stage m_stage = Stage::None;
            std::string m_pendingAuthUser;
            Service::UserService m_userService;
        };

    }
}