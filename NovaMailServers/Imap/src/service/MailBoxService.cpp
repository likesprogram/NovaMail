#include <service/MailBoxService.hpp>
#include <service/UserService.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Imap::Service {
        static const char* ToSqlMappingType(MailBoxService::MailMappingType type) {
            switch (type) {
            case MailBoxService::MailMappingType::From: return "FROM";
            case MailBoxService::MailMappingType::To:   return "TO";
            default:                    return "TO";
            }
        }

        static const char* ToSqlMappingStatus(MailBoxService::MailMappingStatus status) {
            switch (status) {
            case MailBoxService::MailMappingStatus::Normal:    return "NORMAL";
            case MailBoxService::MailMappingStatus::Queued:    return "QUEUED";
            case MailBoxService::MailMappingStatus::Sending:   return "SENDING";
            case MailBoxService::MailMappingStatus::Sent:      return "SENT";
            case MailBoxService::MailMappingStatus::Failed:    return "FAILED";
            default:                           return "NORMAL";
            }
        }

        MailBoxService::MailBoxService(LikeHub::Storage::DatabasePool& pool): IService(pool) { }

        static bool SelectMailBox(LikeHub::Storage::DatabaseConnection& conn, const Address::EmailAddress& emailAddress, Entity::MailBox& mailbox) {
            try {
                auto result = conn->sql(
                    "SELECT m.mailBoxId, m.userId, m.domainNameId, m.localPart, m.isOpen, u.status AS userStatus "
                    "FROM MailBoxes m "
                    "JOIN DomainNames d ON m.domainNameId = d.domainNameId "
                    "JOIN Users u ON m.userId = u.userId "
                    "WHERE d.domainName = ? AND m.localPart = ? "
                    "LIMIT 1;"
                ).bind(emailAddress.GetDomain().Name()).bind(emailAddress.Local()).execute();

                mysqlx::Row row = result.fetchOne();
                if (!row) return false;

                mailbox.mailBoxId    = row[0].get<uint64_t>();
                mailbox.userId       = row[1].get<uint64_t>();
                mailbox.domainNameId = row[2].get<uint64_t>();
                mailbox.localPart    = row[3].get<std::string>();
                mailbox.enabled      = row[4].get<int>() == 1;
                mailbox.userStatus   = UserService::ParseUserStatus(row[5].get<std::string>());

                return true;
            } catch (const std::exception& e) {
                LogError(u"InsertMail failed: {}", e.what());
                return false;
            }
        }
 
        static bool RowToImapFetchMessage(mysqlx::Row row, MailBoxService::ImapFetchMessage& message) {
            if (!row) return false;

            if (!row[0].isNull()) message.mappingId = row[0].get<uint64_t>();
            if (!row[1].isNull()) message.mailBoxId = row[1].get<uint64_t>();
            if (!row[2].isNull()) message.folderId  = row[2].get<uint64_t>();
            if (!row[3].isNull()) message.mailId    = row[3].get<uint64_t>();
            if (!row[4].isNull()) message.imapUid   = row[4].get<uint64_t>();

            if (!row[5].isNull()) message.isRead     = row[5].get<int>() != 0;
            if (!row[6].isNull()) message.isStarred  = row[6].get<int>() != 0;
            if (!row[7].isNull()) message.isAnswered = row[7].get<int>() != 0;
            if (!row[8].isNull()) message.internalDate = static_cast<std::time_t>(row[8].get<int64_t>());

            message.success = message.mappingId != 0 && message.mailBoxId != 0 && message.folderId != 0 && message.mailId != 0 && message.imapUid != 0;

            return message.success;
        }

        MailBoxService::GetMailBoxResult MailBoxService::GetMailBox(const Address::EmailAddress& emailAddress, Entity::MailBox& outMailbox, uint64_t loginUserId) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return GetMailBoxResult::Error;

            NovaMail::Imap::Entity::MailBox mailBox;

            // 查询
            if (!SelectMailBox((*conn), emailAddress, mailBox)) return GetMailBoxResult::NotFound;

            // 数据异常
            if (!mailBox.Valid()) return GetMailBoxResult::Error;

            // 邮箱不可用
            if (!mailBox.enabled || mailBox.userStatus != Entity::UserStatus::Normal) return GetMailBoxResult::Disabled;

            // 所属校验
            if (loginUserId > 0 && loginUserId != mailBox.userId) return GetMailBoxResult::NotOwned;

            // 正常返回
            outMailbox = mailBox;
            return GetMailBoxResult::Normal;
        }

        bool MailBoxService::IsLocalDomain(const Address::Domain& domain) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return false;
            try {
                const std::string domainName = domain.Name();
                if (domainName.empty()) return false;
                auto result = (*conn)->sql(
                    "SELECT domainNameId FROM DomainNames WHERE domainName = ? LIMIT 1;"
                ).bind(domainName).execute();
                mysqlx::Row row = result.fetchOne();
                return static_cast<bool>(row);
            } catch (const std::exception& e) {
                LogError(u"InsertMail failed: {}", e.what());
                return false;
            }
        }

        MailBoxService::ImapMailboxCounter MailBoxService::LoadImapMailboxCounter(uint64_t mailBoxId, uint64_t folderId) {
            ImapMailboxCounter counter;

            if (mailBoxId == 0 || folderId == 0) {
                LogWarn(
                    u"LoadImapMailboxCounter skipped: invalid argument, mailBoxId={} folderId={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId)
                );
                return counter;
            }

            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(
                    u"LoadImapMailboxCounter failed: database connection unavailable, mailBoxId={} folderId={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId)
                );
                return counter;
            }

            try {
                auto countResult = (*conn)->sql(
                    "SELECT COUNT(*) "
                    "FROM MailBoxMailMapping "
                    "WHERE mailBoxId = ? "
                    "  AND folderId = ?;"
                ).bind(mailBoxId)
                 .bind(folderId)
                 .execute();

                auto countRow = countResult.fetchOne();
                if (countRow && !countRow[0].isNull())  counter.exists = countRow[0].get<uint64_t>();

                // UNSEEN 响应码需要的是第一封未读邮件的 sequence number (不是未读数量)
                auto unseenResult = (*conn)->sql(
                    "SELECT seq "
                    "FROM ( "
                    "    SELECT "
                    "        imapUid, "
                    "        isRead, "
                    "        ROW_NUMBER() OVER (ORDER BY imapUid ASC) AS seq "
                    "    FROM MailBoxMailMapping "
                    "    WHERE mailBoxId = ? "
                    "      AND folderId = ? "
                    ") t "
                    "WHERE t.isRead = 0 "
                    "ORDER BY t.seq ASC "
                    "LIMIT 1;"
                ).bind(mailBoxId)
                 .bind(folderId)
                 .execute();

                auto unseenRow = unseenResult.fetchOne();
                if (unseenRow && !unseenRow[0].isNull()) {
                    counter.firstUnseenSeq = unseenRow[0].get<uint64_t>();
                }

                counter.success = true;
                return counter;
            } catch (const std::exception& e) {
                LogError(
                    u"LoadImapMailboxCounter failed: mailBoxId={} folderId={} error={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId),
                    e.what()
                );
                return counter;
            }
        }

        std::optional<MailBoxService::ImapFetchMessage> MailBoxService::GetMessageBySequence(uint64_t mailBoxId, uint64_t folderId, uint64_t sequence) {
            if (mailBoxId == 0 || folderId == 0 || sequence == 0) {
                LogWarn(
                    u"GetMessageBySequence skipped: invalid argument, mailBoxId={} folderId={} sequence={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId),
                    static_cast<int64_t>(sequence)
                );
                return std::nullopt;
            }
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(
                    u"GetMessageBySequence failed: database connection unavailable, mailBoxId={} folderId={} sequence={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId),
                    static_cast<int64_t>(sequence)
                );
                return std::nullopt;
            }
        
            try {
                auto result = (*conn)->sql(
                    "SELECT mappingId, mailBoxId, folderId, mailId, imapUid, isRead, isStarred, isAnswered, "
                    "UNIX_TIMESTAMP(createTime) AS internalDate FROM MailBoxMailMapping "
                    "WHERE mailBoxId = ? AND folderId = ? AND imapUid IS NOT NULL "
                    "ORDER BY imapUid ASC LIMIT 1 OFFSET ?;"
                ).bind(mailBoxId)
                 .bind(folderId)
                 .bind(sequence - 1)
                 .execute();
            
                auto row = result.fetchOne();
                if (!row) {
                    LogWarn(
                        u"GetMessageBySequence skipped: message not found, mailBoxId={} folderId={} sequence={}",
                        static_cast<int64_t>(mailBoxId),
                        static_cast<int64_t>(folderId),
                        static_cast<int64_t>(sequence)
                    );
                    return std::nullopt;
                }
            
                ImapFetchMessage message;
                if (!RowToImapFetchMessage(row, message)) {
                    LogWarn(
                        u"GetMessageBySequence failed: invalid mapping row, mailBoxId={} folderId={} sequence={}",
                        static_cast<int64_t>(mailBoxId),
                        static_cast<int64_t>(folderId),
                        static_cast<int64_t>(sequence)
                    );
                    return std::nullopt;
                }
            
                message.sequence = sequence;
            
                return message;
            } catch (const std::exception& e) {
                LogError(
                    u"GetMessageBySequence failed: mailBoxId={} folderId={} sequence={} error={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId),
                    static_cast<int64_t>(sequence),
                    e.what()
                );
                return std::nullopt;
            }
        }

        std::optional<MailBoxService::ImapFetchMessage> MailBoxService::GetMessageByUid(uint64_t mailBoxId, uint64_t folderId, uint64_t imapUid) {
            if (mailBoxId == 0 || folderId == 0 || imapUid == 0) {
                LogWarn(
                    u"GetMessageByUid skipped: invalid argument, mailBoxId={} folderId={} uid={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId),
                    static_cast<int64_t>(imapUid)
                );
                return std::nullopt;
            }
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(
                    u"GetMessageByUid failed: database connection unavailable, mailBoxId={} folderId={} uid={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId),
                    static_cast<int64_t>(imapUid)
                );
                return std::nullopt;
            }
        
            try {
                auto result = (*conn)->sql(
                    "SELECT mappingId, mailBoxId, folderId, mailId, imapUid, isRead, isStarred, isAnswered "
                    "FROM MailBoxMailMapping "
                    "WHERE mailBoxId = ? "
                    "  AND folderId = ? "
                    "  AND imapUid = ? "
                    "LIMIT 1;"
                ).bind(mailBoxId)
                 .bind(folderId)
                 .bind(imapUid)
                 .execute();
            
                auto row = result.fetchOne();
                if (!row) {
                    LogWarn(
                        u"GetMessageByUid skipped: message not found, mailBoxId={} folderId={} uid={}",
                        static_cast<int64_t>(mailBoxId),
                        static_cast<int64_t>(folderId),
                        static_cast<int64_t>(imapUid)
                    );
                    return std::nullopt;
                }
            
                ImapFetchMessage message;
                if (!RowToImapFetchMessage(row, message)) {
                    LogWarn(
                        u"GetMessageByUid failed: invalid mapping row, mailBoxId={} folderId={} uid={}",
                        static_cast<int64_t>(mailBoxId),
                        static_cast<int64_t>(folderId),
                        static_cast<int64_t>(imapUid)
                    );
                    return std::nullopt;
                }
            
                auto seqResult = (*conn)->sql(
                    "SELECT COUNT(*) "
                    "FROM MailBoxMailMapping "
                    "WHERE mailBoxId = ? "
                    "  AND folderId = ? "
                    "  AND imapUid IS NOT NULL "
                    "  AND imapUid <= ?;"
                ).bind(mailBoxId)
                 .bind(folderId)
                 .bind(imapUid)
                 .execute();
            
                auto seqRow = seqResult.fetchOne();
                if (seqRow && !seqRow[0].isNull()) {
                    message.sequence = seqRow[0].get<uint64_t>();
                }
            
                if (message.sequence == 0) {
                    LogWarn(
                        u"GetMessageByUid failed: invalid sequence calculated, mailBoxId={} folderId={} uid={}",
                        static_cast<int64_t>(mailBoxId),
                        static_cast<int64_t>(folderId),
                        static_cast<int64_t>(imapUid)
                    );
                    return std::nullopt;
                }
            
                return message;
            } catch (const std::exception& e) {
                LogError(
                    u"GetMessageByUid failed: mailBoxId={} folderId={} uid={} error={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId),
                    static_cast<int64_t>(imapUid),
                    e.what()
                );
                return std::nullopt;
            }
        }

        bool MailBoxService::GetMailboxStatus(uint64_t mailBoxId, uint64_t folderId, uint64_t& messages, uint64_t& unseen, uint64_t& recent) {
            messages = 0;
            unseen = 0;
            recent = 0;
            try {
                auto conn = m_dbPool.Acquire();
                if (!conn) {
                    LogError(u"GetMailboxStatus failed: database connection is null");
                    return false;
                }
            
                auto result = (*conn)->sql(
                    "SELECT "
                    "CAST(COUNT(*) AS UNSIGNED) AS messages, "
                    "CAST(COALESCE(SUM(CASE WHEN isRead = 0 THEN 1 ELSE 0 END), 0) AS UNSIGNED) AS unseen "
                    "FROM MailBoxMailMapping "
                    "WHERE mailBoxId = ? AND folderId = ?"
                ).bind(mailBoxId).bind(folderId).execute();
                auto row = result.fetchOne();
            
                if (!row) return false;
                if (!row[0].isNull()) messages = row[0].get<uint64_t>();
                if (!row[1].isNull()) unseen = row[1].get<uint64_t>();
            
                return true;
            } catch (const std::exception& e) {
                LogError(
                    u"GetMailboxStatus exception: mailBoxId={} folderId={} error={}",
                    static_cast<int64_t>(mailBoxId), static_cast<int64_t>(folderId), e.what()
                );
                return false;
            }
        }

        bool MailBoxService::MarkSeen(uint64_t mappingId) {
            if (mappingId == 0) {
                LogWarn(u"MarkSeen skipped: invalid mappingId=0");
                return false;
            }
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(
                    u"MarkSeen failed: database connection unavailable, mappingId={}",
                    static_cast<int64_t>(mappingId)
                );
                return false;
            }
        
            try {
                auto result = (*conn)->sql(
                    "UPDATE MailBoxMailMapping "
                    "SET isRead = 1 "
                    "WHERE mappingId = ? "
                    "LIMIT 1;"
                ).bind(mappingId).execute();
            
                if (result.getAffectedItemsCount() == 0) {
                    LogWarn(
                        u"MarkSeen skipped: mapping not found or already unchanged, mappingId={}",
                        static_cast<int64_t>(mappingId)
                    );
                }
            
                return true;
            } catch (const std::exception& e) {
                LogError(
                    u"MarkSeen failed: mappingId={} error={}",
                    static_cast<int64_t>(mappingId),
                    e.what()
                );
                return false;
            }
        }
        
        MailBoxService::MoveMailResult MailBoxService::MoveMailMapping(uint64_t mappingId, uint64_t targetFolderId) {
            MoveMailResult output;
            output.mappingId = mappingId;
            output.targetMappingId = mappingId;
            output.newFolderId = targetFolderId;
        
            if (mappingId == 0 || targetFolderId == 0) {
                LogError(
                    u"MoveMailMapping failed: invalid argument, mappingId={} targetFolderId={}",
                    static_cast<int64_t>(mappingId),
                    static_cast<int64_t>(targetFolderId)
                );
                return output;
            }
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(
                    u"MoveMailMapping failed: database connection unavailable, mappingId={} targetFolderId={}",
                    static_cast<int64_t>(mappingId),
                    static_cast<int64_t>(targetFolderId)
                );
                return output;
            }
        
            try {
                (*conn)->startTransaction();
            
                auto allocateImapUid = [&](uint64_t targetMailBoxId, uint64_t targetFolderIdValue) -> uint64_t {
                    (*conn)->sql(
                        "INSERT INTO MailBoxFolderState (mailBoxId, folderId, uidNext, uidValidity) "
                        "VALUES (?, ?, 1, GREATEST(1, CAST(CRC32(CONCAT(?, ':', ?)) AS UNSIGNED))) "
                        "ON DUPLICATE KEY UPDATE uidNext = uidNext;"
                    )
                    .bind(targetMailBoxId)
                    .bind(targetFolderIdValue)
                    .bind(targetMailBoxId)
                    .bind(targetFolderIdValue)
                    .execute();
                
                    auto uidResult = (*conn)->sql(
                        "SELECT uidNext "
                        "FROM MailBoxFolderState "
                        "WHERE mailBoxId = ? AND folderId = ? "
                        "FOR UPDATE;"
                    )
                    .bind(targetMailBoxId)
                    .bind(targetFolderIdValue)
                    .execute();
                
                    auto uidRow = uidResult.fetchOne();
                    if (!uidRow) {
                        throw std::runtime_error("MailBoxFolderState row not found after initialization");
                    }
                
                    uint64_t uidNext = uidRow[0].get<uint64_t>();
                
                    if (uidNext == 0 || uidNext > 0xFFFFFFFFull) {
                        throw std::runtime_error("IMAP UID exhausted or invalid");
                    }
                
                    (*conn)->sql(
                        "UPDATE MailBoxFolderState "
                        "SET uidNext = uidNext + 1 "
                        "WHERE mailBoxId = ? AND folderId = ?;"
                    )
                    .bind(targetMailBoxId)
                    .bind(targetFolderIdValue)
                    .execute();
                
                    return uidNext;
                };
            
                auto sourceResult = (*conn)->sql(
                    "SELECT "
                    "mappingId, mailBoxId, folderId, mailId, imapUid, "
                    "isRead, isStarred, isAnswered, mappingType, `status` "
                    "FROM MailBoxMailMapping "
                    "WHERE mappingId = ? "
                    "LIMIT 1 FOR UPDATE;"
                )
                .bind(mappingId)
                .execute();
            
                auto sourceRow = sourceResult.fetchOne();
                if (!sourceRow) {
                    (*conn)->rollback();
                
                    LogWarn(
                        u"MoveMailMapping skipped: source mapping not found, mappingId={} targetFolderId={}",
                        static_cast<int64_t>(mappingId),
                        static_cast<int64_t>(targetFolderId)
                    );
                
                    return output;
                }
            
                const uint64_t sourceMappingId = sourceRow[0].get<uint64_t>();
                const uint64_t mailBoxId = sourceRow[1].get<uint64_t>();
                const uint64_t oldFolderId = sourceRow[2].get<uint64_t>();
                const uint64_t mailId = sourceRow[3].get<uint64_t>();
                const uint64_t oldImapUid = sourceRow[4].get<uint64_t>();
            
                const bool sourceIsRead = !sourceRow[5].isNull() && sourceRow[5].get<int>() != 0;
                const bool sourceIsStarred = !sourceRow[6].isNull() && sourceRow[6].get<int>() != 0;
                const bool sourceIsAnswered = !sourceRow[7].isNull() && sourceRow[7].get<int>() != 0;
            
                output.mappingId = sourceMappingId;
                output.targetMappingId = sourceMappingId;
                output.mailBoxId = mailBoxId;
                output.mailId = mailId;
                output.oldFolderId = oldFolderId;
                output.newFolderId = targetFolderId;
                output.oldImapUid = oldImapUid;
                output.newImapUid = oldImapUid;
            
                if (mailBoxId == 0 || mailId == 0 || oldFolderId == 0 || oldImapUid == 0) {
                    throw std::runtime_error("invalid source mapping row");
                }
            
                if (oldFolderId == targetFolderId) {
                    (*conn)->commit();
                
                    output.success = true;
                    output.moved = false;
                    output.merged = false;
                
                    LogInfo(
                        u"MoveMailMapping no-op: mappingId={} mailBoxId={} folderId={} imapUid={}",
                        static_cast<int64_t>(sourceMappingId),
                        static_cast<int64_t>(mailBoxId),
                        static_cast<int64_t>(oldFolderId),
                        static_cast<int64_t>(oldImapUid)
                    );
                
                    return output;
                }
            
                /*
                 * 防止触发 uniq_mailbox_folder_mail：
                 * 同一个邮箱、同一个目标文件夹、同一封 mailId 如果已经存在，
                 * 就合并状态并删除源 mapping。
                 */
                auto targetExistingResult = (*conn)->sql(
                    "SELECT mappingId, imapUid, isRead, isStarred, isAnswered "
                    "FROM MailBoxMailMapping "
                    "WHERE mailBoxId = ? AND folderId = ? AND mailId = ? "
                    "LIMIT 1 FOR UPDATE;"
                )
                .bind(mailBoxId)
                .bind(targetFolderId)
                .bind(mailId)
                .execute();
            
                auto targetExistingRow = targetExistingResult.fetchOne();
            
                if (targetExistingRow) {
                    const uint64_t targetMappingId = targetExistingRow[0].get<uint64_t>();
                    const uint64_t targetImapUid = targetExistingRow[1].get<uint64_t>();
                
                    const bool targetIsRead = !targetExistingRow[2].isNull() && targetExistingRow[2].get<int>() != 0;
                    const bool targetIsStarred = !targetExistingRow[3].isNull() && targetExistingRow[3].get<int>() != 0;
                    const bool targetIsAnswered = !targetExistingRow[4].isNull() && targetExistingRow[4].get<int>() != 0;
                
                    const bool mergedIsRead = sourceIsRead || targetIsRead;
                    const bool mergedIsStarred = sourceIsStarred || targetIsStarred;
                    const bool mergedIsAnswered = sourceIsAnswered || targetIsAnswered;
                
                    (*conn)->sql(
                        "UPDATE MailBoxMailMapping "
                        "SET isRead = ?, isStarred = ?, isAnswered = ? "
                        "WHERE mappingId = ? "
                        "LIMIT 1;"
                    )
                    .bind(mergedIsRead ? 1 : 0)
                    .bind(mergedIsStarred ? 1 : 0)
                    .bind(mergedIsAnswered ? 1 : 0)
                    .bind(targetMappingId)
                    .execute();
                
                    (*conn)->sql(
                        "DELETE FROM MailBoxMailMapping "
                        "WHERE mappingId = ? "
                        "LIMIT 1;"
                    )
                    .bind(sourceMappingId)
                    .execute();
                
                    (*conn)->commit();
                
                    output.success = true;
                    output.moved = true;
                    output.merged = true;
                    output.targetMappingId = targetMappingId;
                    output.newFolderId = targetFolderId;
                    output.newImapUid = targetImapUid;
                
                    LogInfo(
                        u"MoveMailMapping merged: sourceMappingId={} targetMappingId={} mailBoxId={} mailId={} oldFolderId={} targetFolderId={} oldUid={} targetUid={}",
                        static_cast<int64_t>(sourceMappingId),
                        static_cast<int64_t>(targetMappingId),
                        static_cast<int64_t>(mailBoxId),
                        static_cast<int64_t>(mailId),
                        static_cast<int64_t>(oldFolderId),
                        static_cast<int64_t>(targetFolderId),
                        static_cast<int64_t>(oldImapUid),
                        static_cast<int64_t>(targetImapUid)
                    );
                
                    return output;
                }
            
                const uint64_t newImapUid = allocateImapUid(mailBoxId, targetFolderId);
            
                auto updateResult = (*conn)->sql(
                    "UPDATE MailBoxMailMapping "
                    "SET folderId = ?, imapUid = ? "
                    "WHERE mappingId = ? "
                    "LIMIT 1;"
                )
                .bind(targetFolderId)
                .bind(newImapUid)
                .bind(sourceMappingId)
                .execute();
            
                if (updateResult.getAffectedItemsCount() == 0) {
                    throw std::runtime_error("move mapping update affected 0 rows");
                }
            
                (*conn)->commit();
            
                output.success = true;
                output.moved = true;
                output.merged = false;
                output.targetMappingId = sourceMappingId;
                output.newFolderId = targetFolderId;
                output.newImapUid = newImapUid;
            
                LogInfo(
                    u"MoveMailMapping success: mappingId={} mailBoxId={} mailId={} oldFolderId={} newFolderId={} oldUid={} newUid={}",
                    static_cast<int64_t>(sourceMappingId),
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(mailId),
                    static_cast<int64_t>(oldFolderId),
                    static_cast<int64_t>(targetFolderId),
                    static_cast<int64_t>(oldImapUid),
                    static_cast<int64_t>(newImapUid)
                );
            
                return output;
            } catch (const std::exception& e) {
                try {
                    (*conn)->rollback();
                } catch (...) {
                }
            
                LogError(
                    u"MoveMailMapping failed: mappingId={} targetFolderId={} error={}",
                    static_cast<int64_t>(mappingId),
                    static_cast<int64_t>(targetFolderId),
                    e.what()
                );
            
                return output;
            }
        }

        MailBoxService::MoveMailResult MailBoxService::MoveMailByUid(uint64_t mailBoxId, uint64_t sourceFolderId, uint64_t imapUid, uint64_t targetFolderId) {
            MoveMailResult output;
            output.mailBoxId = mailBoxId;
            output.oldFolderId = sourceFolderId;
            output.oldImapUid = imapUid;
            output.newFolderId = targetFolderId;
        
            if (mailBoxId == 0 || sourceFolderId == 0 || imapUid == 0 || targetFolderId == 0) {
                LogError(
                    u"MoveMailByUid failed: invalid argument, mailBoxId={} sourceFolderId={} uid={} targetFolderId={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(sourceFolderId),
                    static_cast<int64_t>(imapUid),
                    static_cast<int64_t>(targetFolderId)
                );
            
                return output;
            }
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(
                    u"MoveMailByUid failed: database connection unavailable, mailBoxId={} sourceFolderId={} uid={} targetFolderId={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(sourceFolderId),
                    static_cast<int64_t>(imapUid),
                    static_cast<int64_t>(targetFolderId)
                );
            
                return output;
            }
        
            try {
                auto result = (*conn)->sql(
                    "SELECT mappingId "
                    "FROM MailBoxMailMapping "
                    "WHERE mailBoxId = ? "
                    "  AND folderId = ? "
                    "  AND imapUid = ? "
                    "LIMIT 1;"
                )
                .bind(mailBoxId)
                .bind(sourceFolderId)
                .bind(imapUid)
                .execute();
            
                auto row = result.fetchOne();
                if (!row || row[0].isNull()) {
                    LogWarn(
                        u"MoveMailByUid skipped: source message not found, mailBoxId={} sourceFolderId={} uid={} targetFolderId={}",
                        static_cast<int64_t>(mailBoxId),
                        static_cast<int64_t>(sourceFolderId),
                        static_cast<int64_t>(imapUid),
                        static_cast<int64_t>(targetFolderId)
                    );
                
                    return output;
                }
            
                const uint64_t mappingId = row[0].get<uint64_t>();
            
                return MoveMailMapping(mappingId, targetFolderId);
            } catch (const std::exception& e) {
                LogError(
                    u"MoveMailByUid failed: mailBoxId={} sourceFolderId={} uid={} targetFolderId={} error={}",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(sourceFolderId),
                    static_cast<int64_t>(imapUid),
                    static_cast<int64_t>(targetFolderId),
                    e.what()
                );
            
                return output;
            }
        }

        bool MailBoxService::UpdateMessageFlags(uint64_t mappingId, ImapMessageFlag flags) {
            if (mappingId == 0) return false;
        
            const bool seen = HasMessageFlag(flags, ImapMessageFlag::Seen);
            const bool flagged = HasMessageFlag(flags, ImapMessageFlag::Flagged);
            const bool answered = HasMessageFlag(flags, ImapMessageFlag::Answered);
        
            try {
                auto conn = m_dbPool.Acquire();
                if (!conn) {
                    LogError(u"UpdateMessageFlags failed: database connection is null");
                    return false;
                }
            
                (*conn)->sql(
                    "UPDATE MailBoxMailMapping "
                    "SET isRead = ?, isStarred = ?, isAnswered = ? "
                    "WHERE mappingId = ?"
                ).bind(seen ? 1 : 0).bind(flagged ? 1 : 0).bind(answered ? 1 : 0).bind(mappingId).execute();
            
                return true;
            } catch (const std::exception& e) {
                LogError(
                    u"UpdateMessageFlags exception: mappingId={} error={}",
                    static_cast<int64_t>(mappingId), e.what()
                );
            
                return false;
            }
        }

        MailBoxService::MailMappingCreateResult MailBoxService::AddSenderMailMapping(uint64_t mailBoxId, uint64_t mailId, uint64_t folderId, MailMappingStatus status) {
            MailMappingCreateResult output;
            output.mailBoxId = mailBoxId;
            output.mailId = mailId;
            output.folderId = folderId;
            if (mailBoxId == 0 || mailId == 0 || folderId == 0) {
                LogError(
                    u"AddSenderMailMapping failed: invalid argument, mailBoxId={} mailId={} folderId={}",
                    static_cast<int64_t>(mailBoxId), static_cast<int64_t>(mailId), static_cast<int64_t>(folderId)
                );
                return output;
            }
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(u"AddSenderMailMapping failed: database connection unavailable");
                return output;
            }
        
            try {
                (*conn)->startTransaction();
            
                // 给邮箱文件夹分配一个新的 UID
                auto allocateImapUid = [&](uint64_t targetMailBoxId, uint64_t targetFolderId) -> uint64_t {
                    (*conn)->sql(
                        "INSERT INTO MailBoxFolderState (mailBoxId, folderId, uidNext, uidValidity) "
                        "VALUES (?, ?, 1, GREATEST(1, CAST(CRC32(CONCAT(?, ':', ?)) AS UNSIGNED))) "
                        "ON DUPLICATE KEY UPDATE uidNext = uidNext;"
                    ).bind(targetMailBoxId).bind(targetFolderId).bind(targetMailBoxId).bind(targetFolderId).execute();
                
                    auto uidResult = (*conn)->sql(
                        "SELECT uidNext FROM MailBoxFolderState "
                        "WHERE mailBoxId = ? AND folderId = ? FOR UPDATE;"
                    ).bind(targetMailBoxId).bind(targetFolderId).execute();
                
                    auto uidRow = uidResult.fetchOne();
                    if (!uidRow) throw std::runtime_error("MailBoxFolderState row not found after initialization");
                
                    uint64_t uidNext = uidRow[0].get<uint64_t>();
                
                    if (uidNext == 0 || uidNext > 0xFFFFFFFFull) throw std::runtime_error("IMAP UID exhausted or invalid");
                
                    (*conn)->sql(
                        "UPDATE MailBoxFolderState "
                        "SET uidNext = uidNext + 1 "
                        "WHERE mailBoxId = ? AND folderId = ?;"
                    ).bind(targetMailBoxId).bind(targetFolderId).execute();
                
                    return uidNext;
                };
            
                // 查询邮件映射，是否已存在
                auto existingResult = (*conn)->sql(
                    "SELECT mappingId, folderId, imapUid FROM MailBoxMailMapping "
                    "WHERE mailBoxId = ? AND mailId = ? AND mappingType = 'FROM' "
                    "LIMIT 1 FOR UPDATE;"
                ).bind(mailBoxId).bind(mailId).execute();
                auto existingRow = existingResult.fetchOne();
            
                if (existingRow) {
                    const uint64_t mappingId = existingRow[0].get<uint64_t>();
                    const uint64_t oldFolderId = existingRow[1].get<uint64_t>();
                    const uint64_t oldImapUid = existingRow[2].get<uint64_t>();
                    uint64_t newImapUid = oldImapUid;

                    // 文件夹改变或没有 ImapUid 时分配一个新的 UID
                    if (oldFolderId != folderId || oldImapUid == 0) newImapUid = allocateImapUid(mailBoxId, folderId);
                    auto updateResult = (*conn)->sql(
                        "UPDATE MailBoxMailMapping SET `status` = ?, folderId = ?, imapUid = ?, isRead = 1, isStarred = 0, isAnswered = 0 WHERE mappingId = ? LIMIT 1;"
                    ).bind(ToSqlMappingStatus(status)).bind(folderId).bind(newImapUid).bind(mappingId).execute();
                
                    if (updateResult.getAffectedItemsCount() == 0) throw std::runtime_error("sender mapping update affected 0 rows");
                
                    (*conn)->commit();

                    output.success = true;
                    output.updated = true;
                    output.created = false;
                    output.mappingId = mappingId;
                    output.mailBoxId = mailBoxId;
                    output.mailId = mailId;
                    output.folderId = folderId;
                    output.imapUid = newImapUid;

                    LogInfo(
                        u"AddSenderMailMapping updated: mailBoxId={} mailId={} mappingId={} folderId={} imapUid={} status={}",
                        static_cast<int64_t>(mailBoxId), static_cast<int64_t>(mailId), static_cast<int64_t>(mappingId),
                        static_cast<int64_t>(folderId), static_cast<int64_t>(newImapUid), ToSqlMappingStatus(status)
                    );
                
                    return output;
                }
            
                // 获取新的 IMAPUID 并 插入数据
                const uint64_t imapUid = allocateImapUid(mailBoxId, folderId);
                auto insertResult = (*conn)->sql(
                    "INSERT INTO MailBoxMailMapping ("
                    " mailBoxId, mailId, mappingType, `status`, folderId, imapUid, "
                    " isRead, isStarred, isAnswered "
                    ") VALUES ( ?, ?, 'FROM', ?, ?, ?, 1, 0, 0 );"
                ).bind(mailBoxId).bind(mailId).bind(ToSqlMappingStatus(status)).bind(folderId).bind(imapUid).execute();
            
                const uint64_t mappingId = insertResult.getAutoIncrementValue();
            
                if (mappingId == 0) throw std::runtime_error("sender mapping insert returned zero mappingId");
            
                (*conn)->commit();
            
                output.success = true;
                output.created = true;
                output.updated = false;
                output.mappingId = mappingId;
                output.mailBoxId = mailBoxId;
                output.mailId = mailId;
                output.folderId = folderId;
                output.imapUid = imapUid;

                LogInfo(
                    u"AddSenderMailMapping success: mailBoxId={} mailId={} mappingId={} folderId={} imapUid={} status={}",
                    static_cast<int64_t>(mailBoxId), static_cast<int64_t>(mailId), static_cast<int64_t>(mappingId),
                    static_cast<int64_t>(folderId), static_cast<int64_t>(imapUid), ToSqlMappingStatus(status)
                );
            
                return output;
            } catch (const std::exception& e) {
                try { (*conn)->rollback(); } catch (...) { }
            
                LogError(
                    u"AddSenderMailMapping failed: mailBoxId={} mailId={} folderId={} error={}",
                    static_cast<int64_t>(mailBoxId), static_cast<int64_t>(mailId), static_cast<int64_t>(folderId), e.what()
                );
            
                return output;
            }
        }

        bool MailBoxService::UpdateSenderMailStatus(uint64_t mailBoxId, uint64_t mailId, MailMappingStatus status) {
            if (mailBoxId == 0 || mailId == 0) {
                LogError(
                    u"UpdateSenderMailStatus failed: invalid argument, mailBoxId={} mailId={}",
                    static_cast<int64_t>(mailBoxId),static_cast<int64_t>(mailId)
                );
                return false;
            }

            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(u"UpdateSenderMailStatus failed: database connection unavailable");
                return false;
            }

            try {
                auto result = (*conn)->sql(
                    "UPDATE MailBoxMailMapping SET `status` = ? "
                    "WHERE mailBoxId = ? AND mailId = ? AND mappingType = 'FROM' "
                    "LIMIT 1;"
                ).bind(ToSqlMappingStatus(status)).bind(mailBoxId).bind(mailId).execute();

                if (result.getAffectedItemsCount() == 0) {
                    LogWarn(
                        u"UpdateSenderMailStatus skipped: sender mapping not found, mailBoxId={} mailId={} status={}",
                        static_cast<int64_t>(mailBoxId),static_cast<int64_t>(mailId), ToSqlMappingStatus(status)
                    );
                    return false;
                }

                return true;
            } catch (const std::exception& e) {
                LogError(
                    u"UpdateSenderMailStatus failed: mailBoxId={} mailId={} error={}",
                    static_cast<int64_t>(mailBoxId),static_cast<int64_t>(mailId), e.what()
                );
                return false;
            }
        }

        MailBoxService::AddRecipientMappingsResult MailBoxService::AddRecipientMailMappings(uint64_t mailId, const std::vector<RecipientMapping>& recipients, uint64_t defaultInboxFolderId) {
            AddRecipientMappingsResult output;
        
            if (mailId == 0 || defaultInboxFolderId == 0) {
                LogError(
                    u"AddRecipientMailMappings failed: invalid argument, mailId={} defaultInboxFolderId={}",
                    static_cast<int64_t>(mailId), static_cast<int64_t>(defaultInboxFolderId)
                );
                return output;
            }
        
            if (recipients.empty()) {
                LogWarn(
                    u"AddRecipientMailMappings skipped: empty recipients, mailId={}", static_cast<int64_t>(mailId)
                );
                output.success = true;
                return output;
            }
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(u"AddRecipientMailMappings failed: database connection unavailable");
                return output;
            }
        
            try {
                (*conn)->startTransaction();

                // 给邮箱文件夹分配一个新的 UID
                auto allocateImapUid = [&](uint64_t targetMailBoxId, uint64_t targetFolderId) -> uint64_t {
                    (*conn)->sql(
                        "INSERT INTO MailBoxFolderState (mailBoxId, folderId, uidNext, uidValidity) "
                        "VALUES (?, ?, 1, GREATEST(1, CAST(CRC32(CONCAT(?, ':', ?)) AS UNSIGNED))) "
                        "ON DUPLICATE KEY UPDATE uidNext = uidNext;"
                    ).bind(targetMailBoxId).bind(targetFolderId).bind(targetMailBoxId).bind(targetFolderId).execute();
                
                    auto uidResult = (*conn)->sql(
                        "SELECT uidNext FROM MailBoxFolderState "
                        "WHERE mailBoxId = ? AND folderId = ? "
                        "FOR UPDATE;"
                    ).bind(targetMailBoxId) .bind(targetFolderId) .execute();
                
                    auto uidRow = uidResult.fetchOne();
                    if (!uidRow) throw std::runtime_error("MailBoxFolderState row not found after initialization");
                
                    uint64_t uidNext = uidRow[0].get<uint64_t>();
                
                    if (uidNext == 0 || uidNext > 0xFFFFFFFFull) throw std::runtime_error("IMAP UID exhausted or invalid");
                
                    (*conn)->sql(
                        "UPDATE MailBoxFolderState SET uidNext = uidNext + 1 "
                        "WHERE mailBoxId = ? AND folderId = ?;"
                    ).bind(targetMailBoxId).bind(targetFolderId).execute();
                
                    return uidNext;
                };
            
                for (const RecipientMapping& recipient : recipients) {
                    if (recipient.mailBoxId == 0) {
                        LogWarn(u"AddRecipientMailMappings ignored invalid recipient: mailId={} mailBoxId=0", static_cast<int64_t>(mailId));
                        continue;
                    }
                
                    const uint64_t folderId = recipient.folderId == 0 ? defaultInboxFolderId : recipient.folderId;
                
                    if (folderId == 0) {
                        LogWarn(
                            u"AddRecipientMailMappings ignored recipient with invalid folderId: mailId={} mailBoxId={}",
                            static_cast<int64_t>(mailId), static_cast<int64_t>(recipient.mailBoxId)
                        );
                        continue;
                    }
                
                    // 查询是否已有数据
                    const std::string mappingType = ToSqlMappingType(recipient.mappingType);
                    auto existingResult = (*conn)->sql(
                        "SELECT mappingId, folderId, imapUid FROM MailBoxMailMapping "
                        "WHERE mailBoxId = ? AND mailId = ? AND mappingType = ? "
                        "LIMIT 1 FOR UPDATE;"
                    ).bind(recipient.mailBoxId).bind(mailId).bind(mappingType).execute();
                
                    auto existingRow = existingResult.fetchOne();
                    if (existingRow) {
                        const uint64_t mappingId = existingRow[0].get<uint64_t>();
                        const uint64_t oldFolderId = existingRow[1].get<uint64_t>();
                        const uint64_t oldImapUid = existingRow[2].get<uint64_t>();
                        uint64_t newImapUid = oldImapUid;

                        // 文件夹改变或没有 ImapUid 时分配一个新的 UID
                        if (oldFolderId != folderId || oldImapUid == 0) newImapUid = allocateImapUid(recipient.mailBoxId, folderId);
                        auto updateResult = (*conn)->sql(
                            "UPDATE MailBoxMailMapping SET `status` = 'NORMAL', folderId = ?, imapUid = ?, isRead = ?, isStarred = ?, isAnswered = 0 "
                            "WHERE mappingId = ? LIMIT 1;"
                        ).bind(folderId).bind(newImapUid).bind(recipient.isRead ? 1 : 0).bind(recipient.isStarred ? 1 : 0).bind(mappingId).execute();
                    
                        if (updateResult.getAffectedItemsCount() == 0) throw std::runtime_error("recipient mapping update affected 0 rows");
                        
                        MailMappingCreateResult result;
                        result.success = true;
                        result.updated = true;
                        result.created = false;
                        result.mappingId = mappingId;
                        result.mailBoxId = recipient.mailBoxId;
                        result.mailId = mailId;
                        result.folderId = folderId;
                        result.imapUid = newImapUid;
                        output.mailMappings.push_back(result);
                    
                        LogInfo(
                            u"AddRecipientMailMappings updated: mailId={} mailBoxId={} mappingId={} folderId={} imapUid={} mappingType={}",
                            static_cast<int64_t>(mailId), static_cast<int64_t>(recipient.mailBoxId), static_cast<int64_t>(mappingId),
                            static_cast<int64_t>(folderId), static_cast<int64_t>(newImapUid), mappingType
                        );
                        continue;
                    }
                
                    const uint64_t imapUid = allocateImapUid(recipient.mailBoxId, folderId);
                
                    auto insertResult = (*conn)->sql(
                        "INSERT INTO MailBoxMailMapping ( mailBoxId, mailId, mappingType, `status`, folderId, imapUid, isRead, isStarred, isAnswered ) "
                        "VALUES ( ?, ?, ?, 'NORMAL', ?, ?, "
                        "    ?, ?, 0 );"
                    ).bind(recipient.mailBoxId).bind(mailId).bind(mappingType).bind(folderId).bind(imapUid).bind(recipient.isRead ? 1 : 0).bind(recipient.isStarred ? 1 : 0).execute();
                
                    const uint64_t mappingId = insertResult.getAutoIncrementValue();
                    if (mappingId == 0) throw std::runtime_error("recipient mapping insert returned zero mappingId");
                
                    MailMappingCreateResult result;
                    result.success = true;
                    result.created = true;
                    result.updated = false;
                    result.mappingId = mappingId;
                    result.mailBoxId = recipient.mailBoxId;
                    result.mailId = mailId;
                    result.folderId = folderId;
                    result.imapUid = imapUid;
                    output.mailMappings.push_back(result);
                
                    LogInfo(
                        u"AddRecipientMailMappings inserted: mailId={} mailBoxId={} mappingId={} folderId={} imapUid={} mappingType={}",
                        static_cast<int64_t>(mailId), static_cast<int64_t>(recipient.mailBoxId), static_cast<int64_t>(mappingId),
                        static_cast<int64_t>(folderId), static_cast<int64_t>(imapUid), mappingType
                    );
                }
            
                if (output.mailMappings.empty()) {
                    LogWarn(
                        u"AddRecipientMailMappings completed but no valid mapping created: mailId={} recipientCount={}",
                        static_cast<int64_t>(mailId), static_cast<int64_t>(recipients.size())
                    );
                
                    (*conn)->rollback();
                    return output;
                }
            
                (*conn)->commit();
            
                output.success = true;
                return output;
            } catch (const std::exception& e) {
                try { (*conn)->rollback(); } catch (...) { }
            
                LogError(
                    u"AddRecipientMailMappings failed: mailId={} count={} error={}",
                    static_cast<int64_t>(mailId), static_cast<int64_t>(recipients.size()), e.what()
                );
            
                output.success = false;
                output.mailMappings.clear();
                return output;
            }
        }
    }
}
