#include <MailFolderService.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <algorithm>
#include <cctype>

namespace NovaMail {
    namespace Imap::Service {

        MailFolderService::MailFolderService(LikeHub::Storage::DatabasePool& databasePool) : LikeHub::IService(databasePool) {}

        std::vector<MailFolderInfo> MailFolderService::ListMailboxes(uint64_t userId) {
             std::vector<MailFolderInfo> folders;

            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(u"ListMailboxes failed: database connection unavailable, userId={}", static_cast<int64_t>(userId));
                return folders;
            }

            try {
                auto result = (*conn)->sql(
                    "SELECT folderId, userId, folderName, IFNULL(`description`, '') AS description, isSystem "
                    "FROM MailFolder WHERE (userId IS NULL AND isSystem = 1)  OR userId = ? "
                    "ORDER BY CASE UPPER(folderName) WHEN 'INBOX' THEN 1 WHEN 'SENT' THEN 2 WHEN 'DRAFT' THEN 3 "
                    "WHEN 'TRASH' THEN 4 WHEN 'SPAM' THEN 5 WHEN 'RECYCLE' THEN 6 WHEN 'DELETED' THEN 7 "
                    " ELSE 100 END ASC, folderName ASC;"
                ).bind(userId).execute();

                for (;;) {
                    auto row = result.fetchOne();
                    if (!row) break;

                    std::optional<uint64_t> ownerUserId;
                    if (!row[1].isNull()) ownerUserId = row[1].get<uint64_t>();

                    auto folder = BuildFolderInfo(
                        row[0].get<uint64_t>(),
                        ownerUserId,
                        row[2].get<std::string>(),
                        row[3].get<std::string>(),
                        row[4].get<int>() != 0
                    );

                    if (folder.Valid()) folders.emplace_back(std::move(folder));
                }

                LogInfo(
                    u"ListMailboxes success: userId={} count={}",
                    static_cast<int64_t>(userId), static_cast<int64_t>(folders.size())
                );

                return folders;
            } catch (const std::exception& e) {
                LogError(
                    u"ListMailboxes failed: userId={} error={}",
                    static_cast<int64_t>(userId), e.what()
                );
                return {};
            }
        }

        std::optional<MailFolderInfo> MailFolderService::GetMailboxForSelect(uint64_t userId, uint64_t mailBoxId, std::string_view imapName) {
            if (userId == 0 || mailBoxId == 0 || imapName.empty()) {
                LogError(
                    u"GetMailboxForSelect failed: invalid argument, userId={} mailBoxId={} imapName={}",
                    static_cast<int64_t>(userId),
                    static_cast<int64_t>(mailBoxId),
                    std::string(imapName)
                );
                return std::nullopt;
            }
        
            const std::string storageName = ToStorageFolderName(imapName);
        
            auto folder = LoadMailboxDefinitionByStorageName(userId, storageName);
            if (!folder || !folder->Valid()) {
                return std::nullopt;
            }
        
            if (!folder->selectable) {
                LogWarn(
                    u"GetMailboxForSelect skipped: mailbox is not selectable, userId={} mailBoxId={} folderId={}",
                    static_cast<int64_t>(userId),
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folder->folderId)
                );
                return std::nullopt;
            }
        
            if (!LoadOrCreateUidState(mailBoxId, *folder)) {
                return std::nullopt;
            }
        
            return folder;
        }

        std::optional<MailFolderInfo> MailFolderService::GetMailboxForStatus(uint64_t userId, uint64_t mailBoxId, std::string_view imapName) {
            return GetMailboxForSelect(userId, mailBoxId, imapName);
        }

        std::optional<MailFolderInfo> MailFolderService::GetMailboxByFolderId(uint64_t userId, uint64_t mailBoxId, uint64_t folderId, bool loadUidState) {
            if (userId == 0 || folderId == 0) {
                LogError(
                    u"GetMailboxByFolderId failed: invalid argument, userId={} mailBoxId={} folderId={}",
                    static_cast<int64_t>(userId),
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId)
                );
                return std::nullopt;
            }
        
            auto folder = LoadMailboxDefinitionByFolderId(userId, folderId);
            if (!folder || !folder->Valid()) {
                return std::nullopt;
            }
        
            if (loadUidState) {
                if (mailBoxId == 0) {
                    LogError(
                        u"GetMailboxByFolderId failed: mailBoxId required when loadUidState=true, userId={} folderId={}",
                        static_cast<int64_t>(userId),
                        static_cast<int64_t>(folderId)
                    );
                    return std::nullopt;
                }
            
                if (!LoadOrCreateUidState(mailBoxId, *folder)) {
                    return std::nullopt;
                }
            }
        
            return folder;
        }

        bool MailFolderService::EnsureMailboxUidState(uint64_t mailBoxId, uint64_t folderId) {
            if (mailBoxId == 0 || folderId == 0) {
                LogError(
                    u"EnsureMailboxUidState failed: invalid argument, mailBoxId={} folderId={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId)
                );
                return false;
            }
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(
                    u"EnsureMailboxUidState failed: database connection unavailable, mailBoxId={} folderId={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId)
                );
                return false;
            }
        
            try {
                auto result = (*conn)->sql(
                    "INSERT INTO MailBoxFolderState (mailBoxId, folderId, uidNext, uidValidity) "
                    "VALUES (?, ?, 1, GREATEST(1, CAST(CRC32(CONCAT(?, ':', ?)) AS UNSIGNED))) "
                    "ON DUPLICATE KEY UPDATE uidNext = uidNext;"
                ).bind(mailBoxId)
                 .bind(folderId)
                 .bind(mailBoxId)
                 .bind(folderId)
                 .execute();
            
                (void)result;
                return true;
            } catch (const std::exception& e) {
                LogError(
                    u"EnsureMailboxUidState failed: mailBoxId={} folderId={} error={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId),
                    e.what()
                );
                return false;
            }
        }

        std::optional<MailFolderInfo> MailFolderService::LoadMailboxDefinitionByStorageName(uint64_t userId, std::string_view storageName) {
            const std::string normalizedStorageName = ToStorageFolderName(storageName);
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(
                    u"LoadMailboxDefinitionByStorageName failed: database connection unavailable, userId={} folderName={}",
                    static_cast<int64_t>(userId),
                    normalizedStorageName
                );
                return std::nullopt;
            }
        
            try {
                auto result = (*conn)->sql(
                    "SELECT folderId, userId, folderName, IFNULL(`description`, '') AS description, isSystem "
                    "FROM MailFolder "
                    "WHERE "
                    "    (userId IS NULL AND isSystem = 1 AND UPPER(folderName) = UPPER(?)) "
                    "    OR "
                    "    (userId = ? AND UPPER(folderName) = UPPER(?)) "
                    "ORDER BY isSystem DESC "
                    "LIMIT 1;"
                ).bind(normalizedStorageName)
                 .bind(userId)
                 .bind(normalizedStorageName)
                 .execute();
            
                auto row = result.fetchOne();
                if (!row) {
                    LogWarn(
                        u"LoadMailboxDefinitionByStorageName skipped: folder not found, userId={} folderName={}",
                        static_cast<int64_t>(userId),
                        normalizedStorageName
                    );
                    return std::nullopt;
                }
            
                std::optional<uint64_t> ownerUserId;
                if (!row[1].isNull()) {
                    ownerUserId = row[1].get<uint64_t>();
                }
            
                auto folder = BuildFolderInfo(
                    row[0].get<uint64_t>(),
                    ownerUserId,
                    row[2].get<std::string>(),
                    row[3].get<std::string>(),
                    row[4].get<int>() != 0
                );
            
                if (!folder.Valid()) {
                    LogWarn(
                        u"LoadMailboxDefinitionByStorageName failed: invalid folder record, userId={} folderName={}",
                        static_cast<int64_t>(userId),
                        normalizedStorageName
                    );
                    return std::nullopt;
                }
            
                return folder;
            } catch (const std::exception& e) {
                LogError(
                    u"LoadMailboxDefinitionByStorageName failed: userId={} folderName={} error={}",
                    static_cast<int64_t>(userId),
                    normalizedStorageName,
                    e.what()
                );
                return std::nullopt;
            }
        }

        std::optional<MailFolderInfo> MailFolderService::LoadMailboxDefinitionByFolderId(uint64_t userId, uint64_t folderId ) {
            if (folderId == 0) {
                return std::nullopt;
            }
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(
                    u"LoadMailboxDefinitionByFolderId failed: database connection unavailable, userId={} folderId={}",
                    static_cast<int64_t>(userId),
                    static_cast<int64_t>(folderId)
                );
                return std::nullopt;
            }
        
            try {
                auto result = (*conn)->sql(
                    "SELECT folderId, userId, folderName, IFNULL(`description`, '') AS description, isSystem "
                    "FROM MailFolder "
                    "WHERE folderId = ? "
                    "  AND ("
                    "        (userId IS NULL AND isSystem = 1) "
                    "        OR userId = ?"
                    "      ) "
                    "LIMIT 1;"
                ).bind(folderId)
                 .bind(userId)
                 .execute();
            
                auto row = result.fetchOne();
                if (!row) {
                    LogWarn(
                        u"LoadMailboxDefinitionByFolderId skipped: folder not found or not owned, userId={} folderId={}",
                        static_cast<int64_t>(userId),
                        static_cast<int64_t>(folderId)
                    );
                    return std::nullopt;
                }
            
                std::optional<uint64_t> ownerUserId;
                if (!row[1].isNull()) {
                    ownerUserId = row[1].get<uint64_t>();
                }
            
                auto folder = BuildFolderInfo(
                    row[0].get<uint64_t>(),
                    ownerUserId,
                    row[2].get<std::string>(),
                    row[3].get<std::string>(),
                    row[4].get<int>() != 0
                );
            
                if (!folder.Valid()) {
                    LogWarn(
                        u"LoadMailboxDefinitionByFolderId failed: invalid folder record, userId={} folderId={}",
                        static_cast<int64_t>(userId),
                        static_cast<int64_t>(folderId)
                    );
                    return std::nullopt;
                }
            
                return folder;
            } catch (const std::exception& e) {
                LogError(
                    u"LoadMailboxDefinitionByFolderId failed: userId={} folderId={} error={}",
                    static_cast<int64_t>(userId),
                    static_cast<int64_t>(folderId),
                    e.what()
                );
                return std::nullopt;
            }
        }

        bool MailFolderService::LoadOrCreateUidState(uint64_t mailBoxId, MailFolderInfo& folder) {
            if (mailBoxId == 0 || folder.folderId == 0) {
                LogError(
                    u"LoadOrCreateUidState failed: invalid argument, mailBoxId={} folderId={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folder.folderId)
                );
                return false;
            }
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(
                    u"LoadOrCreateUidState failed: database connection unavailable, mailBoxId={} folderId={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folder.folderId)
                );
                return false;
            }
        
            try {
                (*conn)->sql(
                    "INSERT INTO MailBoxFolderState (mailBoxId, folderId, uidNext, uidValidity) "
                    "VALUES (?, ?, 1, GREATEST(1, CAST(CRC32(CONCAT(?, ':', ?)) AS UNSIGNED))) "
                    "ON DUPLICATE KEY UPDATE uidNext = uidNext;"
                ).bind(mailBoxId)
                 .bind(folder.folderId)
                 .bind(mailBoxId)
                 .bind(folder.folderId)
                 .execute();
            
                auto result = (*conn)->sql(
                    "SELECT uidNext, uidValidity "
                    "FROM MailBoxFolderState "
                    "WHERE mailBoxId = ? AND folderId = ? "
                    "LIMIT 1;"
                ).bind(mailBoxId)
                 .bind(folder.folderId)
                 .execute();
            
                auto row = result.fetchOne();
                if (!row) {
                    LogWarn(
                        u"LoadOrCreateUidState failed: state not found after init, mailBoxId={} folderId={}",
                        static_cast<int64_t>(mailBoxId),
                        static_cast<int64_t>(folder.folderId)
                    );
                    return false;
                }
            
                folder.uidNext = row[0].get<uint64_t>();
                folder.uidValidity = row[1].get<uint64_t>();
            
                if (folder.uidNext == 0 || folder.uidNext > 0xFFFFFFFFull) {
                    LogError(
                        u"LoadOrCreateUidState failed: invalid uidNext, mailBoxId={} folderId={} uidNext={}",
                        static_cast<int64_t>(mailBoxId),
                        static_cast<int64_t>(folder.folderId),
                        static_cast<int64_t>(folder.uidNext)
                    );
                    return false;
                }
            
                if (folder.uidValidity == 0 || folder.uidValidity > 0xFFFFFFFFull) {
                    LogError(
                        u"LoadOrCreateUidState failed: invalid uidValidity, mailBoxId={} folderId={} uidValidity={}",
                        static_cast<int64_t>(mailBoxId),
                        static_cast<int64_t>(folder.folderId),
                        static_cast<int64_t>(folder.uidValidity)
                    );
                    return false;
                }
            
                folder.hasUidState = true;
                return true;
            } catch (const std::exception& e) {
                LogError(
                    u"LoadOrCreateUidState failed: mailBoxId={} folderId={} error={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folder.folderId),
                    e.what()
                );
                return false;
            }
        }

        MailFolderInfo MailFolderService::BuildFolderInfo(uint64_t folderId, std::optional<uint64_t> userId, std::string storageName, std::string description, bool isSystem) const {
            MailFolderInfo info;
                
            info.folderId = folderId;
            info.userId = userId;
            info.storageName = std::move(storageName);
            info.imapName = ToImapMailboxName(info.storageName);
            info.description = std::move(description);
            info.isSystem = isSystem;
                
            info.selectable = true;
            info.hasChildren = false;
            info.subscribed = true;
            info.specialUse = ResolveSpecialUse(info.storageName);
                
            info.uidNext = 1;
            info.uidValidity = 1;
            info.hasUidState = false;
                
            return info;
        }

        bool MailFolderService::IsSystemFolderName(std::string_view folderName) const {
            const std::string name = ToUpperAscii(folderName);
            return name == "DRAFT" || name == "INBOX" || name == "SENT" || name == "TRASH" || name == "SPAM" || name == "RECYCLE" || name == "DELETED";
        }

        std::string MailFolderService::ToImapMailboxName(std::string_view storageName) const {
            const std::string name = ToUpperAscii(storageName);
            if (name == "INBOX") return "INBOX";
            if (name == "DRAFT") return "Drafts";
            if (name == "SENT") return "Sent";
            if (name == "TRASH") return "Trash";
            if (name == "SPAM") return "Spam";
            if (name == "RECYCLE") return "Recycle";
            if (name == "DELETED")  return "Deleted";
            return std::string(storageName);
        }

        std::string MailFolderService::ToStorageFolderName(std::string_view imapName) const {
            const std::string name = ToUpperAscii(imapName);
            if (name == "INBOX") return "INBOX";
            if (name == "DRAFT" || name == "DRAFTS") return "DRAFT";
            if (name == "SENT") return "SENT";
            if (name == "TRASH") return "TRASH";
            if (name == "SPAM" || name == "JUNK") return "SPAM";
            if (name == "RECYCLE") return "RECYCLE";
            if (name == "DELETED") return "DELETED";
            return std::string(imapName);
        }

        ImapSpecialUse MailFolderService::ResolveSpecialUse(std::string_view storageName) const {
            const std::string name = ToUpperAscii(storageName);
            if (name == "INBOX") return ImapSpecialUse::Inbox;
            if (name == "DRAFT") return ImapSpecialUse::Drafts;
            if (name == "SENT") return ImapSpecialUse::Sent;
            if (name == "TRASH" || name == "RECYCLE" || name == "DELETED") return ImapSpecialUse::Trash;
            if (name == "SPAM") return ImapSpecialUse::Junk;
            return ImapSpecialUse::None;
        }

        std::string MailFolderService::ToUpperAscii(std::string_view value) {
            std::string out;
            out.reserve(value.size());
            for (char ch : value) out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            return out;
        }

        bool MailFolderService::EqualsIgnoreCase(std::string_view lhs, std::string_view rhs) {
            if (lhs.size() != rhs.size()) return false;
            for (size_t i = 0; i < lhs.size(); ++i) {
                const unsigned char l = static_cast<unsigned char>(lhs[i]);
                const unsigned char r = static_cast<unsigned char>(rhs[i]);
                if (std::toupper(l) != std::toupper(r)) return false;
            }
            return true;
        }

    }
}