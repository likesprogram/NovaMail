#pragma once
#include <command/CommandHandler.hpp>
#include <service/MailFolderService.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace NovaMail {
    namespace Imap {
        class AppendCommand final : public AuthenticatedCommandHandler {
        public:
            AppendCommand()
                : m_folderService(LikeHub::Storage::DatabasePool::Instance()) { }

            void Handle(
                const std::shared_ptr<Session>& session,
                const Command& command
            ) override;

            InputResult InputData(
                const std::shared_ptr<Session>& session,
                LikesProgram::Net::Buffer& in
            ) override;

        private:
            enum class Stage : uint8_t {
                None = 0,
                ReadingLiteral
            };

            struct LiteralSpec {
                size_t size{0};
                bool nonSync{false}; // {size+}
            };

            struct AppendArguments {
                std::string mailboxName;
                std::vector<std::string> flags;
                std::optional<std::string> internalDate;
                LiteralSpec literal;
            };

        private:
            static constexpr size_t kMaxAppendLiteralBytes = 64 * 1024 * 1024; // 64MB，可按配置调整

        private:
            bool ParseAppendArguments(
                std::string_view args,
                AppendArguments& out
            ) const;

            bool ReadAString(
                std::string_view text,
                size_t& pos,
                std::string& out
            ) const;

            bool ReadQuotedString(
                std::string_view text,
                size_t& pos,
                std::string& out
            ) const;

            bool ReadFlagList(
                std::string_view text,
                size_t& pos,
                std::vector<std::string>& flags
            ) const;

            bool ReadLiteralSpec(
                std::string_view text,
                size_t& pos,
                LiteralSpec& literal
            ) const;

            void SkipSpaces(
                std::string_view text,
                size_t& pos
            ) const noexcept;

            bool BeginAppendLiteral(
                const std::shared_ptr<Session>& session
            );

            InputResult ReadAppendLiteral(
                const std::shared_ptr<Session>& session,
                LikesProgram::Net::Buffer& in
            );

            void ExecuteAppend(
                const std::shared_ptr<Session>& session
            );

            bool IsSentMailbox(std::string_view mailboxName) const;
            bool IsDraftsMailbox(std::string_view mailboxName) const;

            static std::string ToUpperAscii(std::string_view value);
            static std::string TrimCopy(std::string_view value);

            void ResetState();

        private:
            Service::MailFolderService m_folderService;

            Stage m_stage{Stage::None};

            std::string m_tag;
            AppendArguments m_args;

            size_t m_expectedLiteralBytes{0};
            size_t m_receivedLiteralBytes{0};
            std::string m_literalBuffer;
        };
    }
}