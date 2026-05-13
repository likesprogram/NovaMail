#pragma once

#include <command/CommandHandler.hpp>
#include <service/MailFolderService.hpp>
#include <service/MailBoxService.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace NovaMail {
    namespace Imap {
        class StatusCommand final : public AuthenticatedCommandHandler {
        public:
            StatusCommand()
                : m_folderService(LikeHub::Storage::DatabasePool::Instance()),
                  m_mailBoxService(LikeHub::Storage::DatabasePool::Instance()) { }

            void Handle(
                const std::shared_ptr<Session>& session,
                const Command& command
            ) override;

        private:
            struct StatusArguments {
                std::string mailboxName;
                std::vector<std::string> items;
            };

            struct MailboxStatus {
                uint64_t messages{0};
                uint64_t recent{0};
                uint64_t uidNext{1};
                uint64_t uidValidity{932632908};
                uint64_t unseen{0};
            };

        private:
            bool ParseStatusArguments(
                std::string_view args,
                StatusArguments& out
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

            bool ReadStatusItemList(
                std::string_view text,
                size_t& pos,
                std::vector<std::string>& out
            ) const;

            std::string BuildStatusResponse(
                const std::string& mailboxName,
                const std::vector<std::string>& items,
                const MailboxStatus& status
            ) const;

            void SkipSpaces(
                std::string_view text,
                size_t& pos
            ) const noexcept;

            static bool IsKnownStatusItem(std::string_view item);
            static bool EqualsMailboxName(std::string_view lhs, std::string_view rhs);
            static std::string QuoteString(std::string_view value);
            static std::string ToUpperAscii(std::string_view value);
            static std::string TrimCopy(std::string_view value);

        private:
            Service::MailFolderService m_folderService;
            Service::MailBoxService m_mailBoxService;
        };

    }
}