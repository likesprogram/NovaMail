#pragma once
#include <LikeHub/service/Service.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace NovaMail {
    namespace Imap::Service {
        enum class ImapMessageFlag : uint32_t {
            None     = 0,
            Seen     = 1u << 0,  // \Seen
            Answered = 1u << 1,  // \Answered
            Flagged  = 1u << 2,  // \Flagged
            // Deleted  = 1u << 3,  // \Deleted
            // Draft    = 1u << 4   // \Draft
        };
        inline ImapMessageFlag operator|(ImapMessageFlag lhs, ImapMessageFlag rhs) noexcept {
            return static_cast<ImapMessageFlag>(
                static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
            );
        }
        inline ImapMessageFlag& operator|=(ImapMessageFlag& lhs, ImapMessageFlag rhs) noexcept {
            lhs = lhs | rhs;
            return lhs;
        }
        inline ImapMessageFlag operator&(ImapMessageFlag lhs, ImapMessageFlag rhs) noexcept {
            return static_cast<ImapMessageFlag>(
                static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)
            );
        }
        inline ImapMessageFlag operator~(ImapMessageFlag value) noexcept {
            return static_cast<ImapMessageFlag>(~static_cast<uint32_t>(value));
        }
        inline bool HasMessageFlag(ImapMessageFlag value, ImapMessageFlag flag) noexcept {
            return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
        }
        inline void RemoveMessageFlag(ImapMessageFlag& value, ImapMessageFlag flag) noexcept {
            value = static_cast<ImapMessageFlag>(
                static_cast<uint32_t>(value) & ~static_cast<uint32_t>(flag)
            );
        }

        enum class ImapSpecialUse : uint32_t {
            None    = 0,
            Inbox   = 1u << 0,
            Drafts  = 1u << 1,
            Sent    = 1u << 2,
            Trash   = 1u << 3,
            Junk    = 1u << 4,
            Archive = 1u << 5,
            All     = 1u << 6,
            Flagged = 1u << 7
        };

        inline ImapSpecialUse operator|(ImapSpecialUse lhs, ImapSpecialUse rhs) noexcept {
            return static_cast<ImapSpecialUse>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
        }

        inline ImapSpecialUse& operator|=(ImapSpecialUse& lhs, ImapSpecialUse rhs) noexcept {
            lhs = lhs | rhs;
            return lhs;
        }

        inline bool HasSpecialUse(ImapSpecialUse value, ImapSpecialUse flag) noexcept {
            return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
        }

        struct MailFolderInfo {
            uint64_t folderId{0};
            std::optional<uint64_t> userId;
            // 数据库存储名称：DRAFT / INBOX / SENT / TRASH / SPAM / RECYCLE / DELETED
            std::string storageName;
            // IMAP 对外显示名称：Drafts / INBOX / Sent / Trash / Spam / Recycle / Deleted
            std::string imapName;
            std::string description;

            bool isSystem{false};
            bool selectable{true};
            bool hasChildren{false};
            bool subscribed{true};

            ImapSpecialUse specialUse{ImapSpecialUse::None};

            // SELECT / EXAMINE / STATUS 使用。
            uint64_t uidNext{1};
            uint64_t uidValidity{1};
            bool hasUidState{false};

            bool Valid() const noexcept {
                return folderId != 0 && !storageName.empty() && !imapName.empty();
            }
        };

        class MailFolderService : public LikeHub::IService {
        public:
            explicit MailFolderService(LikeHub::Storage::DatabasePool& databasePool);

            // LIST 专用，无 UID 相关信息
            std::vector<MailFolderInfo> ListMailboxes(uint64_t userId);

            // SELECT / EXAMINE / STATUS 专用，根据 IMAP mailbox 名称查找文件夹，并加载 uidNext / uidValidity
            std::optional<MailFolderInfo> GetMailboxForSelect(uint64_t userId, uint64_t mailBoxId, std::string_view imapName);

            // STATUS 调用方语义清晰
            std::optional<MailFolderInfo> GetMailboxForStatus(uint64_t userId, uint64_t mailBoxId, std::string_view imapName);

            // FETCH / UID FETCH / IDLE / Hub 通知场景使用
            std::optional<MailFolderInfo> GetMailboxByFolderId(uint64_t userId, uint64_t mailBoxId, uint64_t folderId, bool loadUidState);

            // 投递、APPEND、MOVE、COPY 前可以调用，确保 mailBoxId + folderId 的 UID 状态存在
            bool EnsureMailboxUidState(uint64_t mailBoxId, uint64_t folderId);

            bool IsSystemFolderName(std::string_view folderName) const;

            std::string ToImapMailboxName(std::string_view storageName) const;
            std::string ToStorageFolderName(std::string_view imapName) const;

            ImapSpecialUse ResolveSpecialUse(std::string_view storageName) const;

        private:
            static std::string ToUpperAscii(std::string_view value);
            static bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs);
            std::optional<MailFolderInfo> LoadMailboxDefinitionByStorageName(
                uint64_t userId,
                std::string_view storageName
            );

            std::optional<MailFolderInfo> LoadMailboxDefinitionByFolderId(
                uint64_t userId,
                uint64_t folderId
            );

            bool LoadOrCreateUidState(
                uint64_t mailBoxId,
                MailFolderInfo& folder
            );

            MailFolderInfo BuildFolderInfo(uint64_t folderId, std::optional<uint64_t> userId, std::string storageName, std::string description, bool isSystem) const;
        };

    }
}