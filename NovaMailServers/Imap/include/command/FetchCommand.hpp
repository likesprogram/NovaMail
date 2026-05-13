#pragma once
#include <command/CommandHandler.hpp>
#include <service/MailBoxService.hpp>
#include <service/EmailService.hpp>
#include <LikesProgram/net/Buffer.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace NovaMail {
    namespace Imap {
        struct FetchRange {
            uint64_t first{0};
            uint64_t last{0}; // UINT64_MAX 表示 *
        };

        enum class FetchItemType : uint8_t {
            Flags = 0,
            Uid,
            Rfc822Size,
            InternalDate,
            Envelope,
            BodyStructure,

            BodyFull,
            BodyHeader,
            BodyText,
            BodyHeaderFields
        };

        struct FetchItem {
            FetchItemType type{FetchItemType::Flags};

            // BODY.PEEK[...] 不设置 \Seen
            bool peek{true};

            // 响应里使用的 item 名称，例如 BODY[] / BODY[HEADER] / RFC822
            std::string responseName;

            // HEADER.FIELDS 使用
            std::vector<std::string> headerFields;
        };

        struct FetchRequest {
            bool uidMode{false};
        
            // 普通 FETCH 使用展开后的 sequence 列表
            std::vector<uint64_t> sequences;
        
            // UID FETCH 使用 UID 范围，不直接展开
            std::vector<FetchRange> uidRanges;
        
            bool requiresMailData{false};
            bool shouldMarkSeen{false};
        
            std::vector<FetchItem> items;
        };

        class FetchCommand final : public SelectedCommandHandler {
        public:
            FetchCommand();

            void Handle(
                const std::shared_ptr<Session>& session,
                const Command& command
            ) override;

        private:



            struct ParsedMailView {
                std::string raw;
                std::string header;
                std::string body;
            };

        private:
            bool ParseFetchRequest(std::string_view args, uint64_t selectedExists, bool uidMode, FetchRequest& request) const;

            bool ParseSequenceSet(std::string_view value, uint64_t selectedExists, bool uidMode, std::vector<uint64_t>& sequences, std::vector<FetchRange>& uidRanges) const;

            bool UidInRanges(uint64_t uid, const std::vector<FetchRange>& ranges) const;

            void EnsureUidItem(FetchRequest& request) const;

            bool ParseFetchItems(std::string_view value, FetchRequest& request) const;

            bool LoadMessageData(uint64_t mailId, LikesProgram::Net::Buffer& mailData);

            ParsedMailView BuildMailView(const LikesProgram::Net::Buffer& mailData) const;

            std::string BuildFetchResponse(uint64_t sequence, const Service::MailBoxService::ImapFetchMessage& message, const FetchRequest& request, const ParsedMailView* mailView, size_t rawSize) const;

            void AppendFetchItem(std::string& response, const FetchItem& item, const Service::MailBoxService::ImapFetchMessage& message, const ParsedMailView* mailView, size_t rawSize) const;

            std::string BuildFlags(const Service::MailBoxService::ImapFetchMessage& message) const;

            std::string BuildEnvelope(const ParsedMailView* mailView) const;

            std::string BuildBodyStructure(const ParsedMailView* mailView, size_t rawSize) const;

            std::string ExtractHeaderFields(const std::string& header, const std::vector<std::string>& fields) const;

            static std::string GetHeaderValue(const std::string& header, std::string_view fieldName);

            static std::string BuildAddressList(const std::string& value);

            static std::string QuoteNString(std::string_view value);

            static std::string QuoteAString(std::string_view value);

            static std::string ToUpperAscii(std::string_view value);
            static std::string TrimCopy(std::string_view value);
            static bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs);
            static bool StartsWithIgnoreCase(std::string_view value, std::string_view prefix);
            static bool ContainsIgnoreCase(std::string_view value, std::string_view needle);
            static size_t CountLines(std::string_view value);

        private:
            Service::MailBoxService m_mailBoxService;
            Service::EmailService m_emailService;
        };
    }
}