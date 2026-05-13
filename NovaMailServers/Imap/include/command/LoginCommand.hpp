#pragma once
#include <command/CommandHandler.hpp>
#include <service/UserService.hpp>
#include <string>

namespace NovaMail {
    namespace Imap {
        class LoginCommand : public NotAuthenticatedCommandHandler {
        public:
            LoginCommand(): m_userService(LikeHub::Storage::DatabasePool::Instance()) { }
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;
            InputResult InputData(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in) override;
        private:
            enum class Stage {
                None,
                NeedAccount,
                ReadingAccountLiteral,
                NeedPassword,
                ReadingPasswordLiteral,
                NeedCommandEnd
            };
            enum class ReadStringResult {
                Ok,
                NeedMoreData,
                NeedLiteral,
                Bad
            };
            InputResult ConsumeCommandEndAndFinishLogin(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in);
            void ResetLoginState();
            void SkipSpaces(const std::string& text, size_t& pos) const;
            bool ParseLiteralSpec(const std::string& text, size_t& pos, size_t& literalSize, bool& nonSyncLiteral) const;
            ReadStringResult ReadLoginStringFromLine(const std::string& text, size_t& pos, std::string& out, size_t& literalSize, bool& nonSyncLiteral);
            bool BeginLiteralRead(const std::shared_ptr<Session>& session, const std::string& tag, Stage literalStage, size_t literalSize, bool nonSyncLiteral);
            InputResult ReadLiteralBytes(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in);
            InputResult ParsePasswordFromBuffer(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in);
            bool TryFinishLogin(const std::shared_ptr<Session>& session, const std::string& tag);
            bool VerifyAndLogin(const std::shared_ptr<Session>& session, const std::string& tag, const std::string& account, const std::string& password);
        private:
            static constexpr size_t kMaxLoginLiteralBytes = 64 * 1024;
            static constexpr size_t kMaxNonSyncLiteralBytes = 4096;
            Service::UserService m_userService;
            Stage m_stage = Stage::None;

            std::string m_account;
            std::string m_password;

            size_t m_expectedLiteralBytes = 0;
            size_t m_receivedLiteralBytes = 0;
            std::string m_literalBuffer;
        };

    }
}