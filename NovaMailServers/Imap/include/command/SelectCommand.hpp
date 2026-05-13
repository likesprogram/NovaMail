#pragma once
#include <command/CommandHandler.hpp>
#include <command/Command.hpp>
#include <service/MailFolderService.hpp>
#include <service/MailBoxService.hpp>
#include <LikesProgram/net/Buffer.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace NovaMail {
    namespace Imap {
        class SelectCommand final : public AuthenticatedCommandHandler {
        public:
            explicit SelectCommand(bool readOnly): m_readOnly(readOnly),
            m_mailFolderService(LikeHub::Storage::DatabasePool::Instance()), m_mailBoxService(LikeHub::Storage::DatabasePool::Instance()) { }

            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;

            InputResult InputData(
                const std::shared_ptr<Session>& session,
                LikesProgram::Net::Buffer& in
            ) override;

        private:
            enum class Stage : uint8_t {
                None = 0,
                ReadingMailboxLiteral,
                NeedRestLine
            };

            enum class ReadStringResult : uint8_t {
                Ok = 0,
                NeedLiteral,
                Bad
            };

            struct LiteralSpec {
                size_t size{0};
                bool nonSync{false};
            };

            struct SelectOptions {
                bool condStore{false};
                bool qresync{false};
                bool unsupported{false};
                std::string raw;
            };

            struct MailboxStatus {
                uint64_t exists{0};
                uint64_t uidNext{1};
                uint64_t uidValidity{1};

                // 注意：UNSEEN 响应码里需要的是第一封未读邮件的 sequence number，不是未读数量。
                uint64_t firstUnseenSeq{0};
            };

        private:
            static constexpr size_t kMaxMailboxLiteralBytes = 4096;
            static constexpr size_t kMaxSelectRestLineBytes = 8192;

        private:
            bool ParseInitialArguments(
                const std::string& args,
                std::string& mailboxName,
                LiteralSpec& literal,
                SelectOptions& options,
                bool& needLiteral
            ) const;

            bool ParseRestLineAfterLiteral(
                const std::string& line,
                SelectOptions& options
            ) const;

            ReadStringResult ReadMailboxName(
                const std::string& text,
                size_t& pos,
                std::string& out,
                LiteralSpec& literal
            ) const;

            bool ParseLiteralSpec(
                const std::string& text,
                size_t& pos,
                LiteralSpec& literal
            ) const;

            bool ParseSelectOptions(
                const std::string& text,
                size_t& pos,
                SelectOptions& options
            ) const;

            bool ReadBalancedParentheses(
                const std::string& text,
                size_t& pos,
                std::string& out
            ) const;

            bool BeginLiteralRead(
                const std::shared_ptr<Session>& session,
                const LiteralSpec& literal
            );

            InputResult ReadMailboxLiteralBytes(
                const std::shared_ptr<Session>& session,
                LikesProgram::Net::Buffer& in
            );

            InputResult ReadRestLineAndExecute(
                const std::shared_ptr<Session>& session,
                LikesProgram::Net::Buffer& in
            );

            void ExecuteSelect(
                const std::shared_ptr<Session>& session
            );

            MailboxStatus LoadMailboxStatus(uint64_t mailBoxId, const Service::MailFolderInfo& folder);

            bool HasUnsupportedSelectOptions(
                const SelectOptions& options
            ) const noexcept;

            void SkipSpaces(const std::string& text, size_t& pos) const;

            static std::string ToUpperAscii(std::string_view value);
            static bool ContainsWordIgnoreCase(std::string_view text, std::string_view word);

            void ResetState();

        private:
            bool m_readOnly{false};
            Service::MailFolderService m_mailFolderService;
            Service::MailBoxService m_mailBoxService;
            Stage m_stage{Stage::None};
            CommandVerb m_activeVerb{CommandVerb::Unknown};

            std::string m_tag;
            std::string m_mailboxName;
            SelectOptions m_options;

            size_t m_expectedLiteralBytes{0};
            size_t m_receivedLiteralBytes{0};
            std::string m_literalBuffer;
        };

    }
}