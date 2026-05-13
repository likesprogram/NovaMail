#pragma once
#include <command/CommandHandler.hpp>
#include <service/MailBoxService.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace NovaMail {
    namespace Imap {
        class StoreCommand final : public SelectedCommandHandler {
        public:
            StoreCommand(): m_mailBoxService(LikeHub::Storage::DatabasePool::Instance()) { }

            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;

        private:
            enum class StoreMode : uint8_t {
                Replace = 0,
                Add,
                Remove
            };

            struct StoreArguments {
                std::string idSet;
                StoreMode mode{StoreMode::Replace};
                bool silent{false};
                std::vector<std::string> flags;
            };

            struct IdRange {
                uint64_t first{0};
                uint64_t last{0}; // UINT64_MAX 表示 *
            };

        private:
            bool ParseStoreArguments(std::string_view args, StoreArguments& out) const;

            bool ParseStoreOperation(std::string_view token, StoreMode& mode, bool& silent) const;

            bool ReadFlagList(std::string_view text, size_t& pos, std::vector<std::string>& flags) const;

            bool ParseIdSet(std::string_view value, bool uidMode, uint64_t selectedExists, std::vector<uint64_t>& sequences, std::vector<IdRange>& uidRanges) const;

            bool UidInRanges(uint64_t uid, const std::vector<IdRange>& ranges, uint64_t highestUid) const;

            Service::ImapMessageFlag BuildCurrentFlags(const Service::MailBoxService::ImapFetchMessage& message) const;

            void ApplyFlagOperation(Service::ImapMessageFlag& target, const std::vector<std::string>& flags, StoreMode mode) const;

            std::string BuildFlagsResponse(const Service::ImapMessageFlag& flags) const;

            void SkipSpaces(std::string_view text, size_t& pos) const noexcept;

            static std::string ToUpperAscii(std::string_view value);
            static std::string TrimCopy(std::string_view value);

        private:
            Service::MailBoxService m_mailBoxService;
        };

    }
}