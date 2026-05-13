#include <service/MailBoxService.hpp>
#include <service/UserService.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Smtp::Service {
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
 
        MailBoxService::GetMailBoxResult MailBoxService::GetMailBox(const Address::EmailAddress& emailAddress, Entity::MailBox& outMailbox, uint64_t loginUserId) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return GetMailBoxResult::Error;

            NovaMail::Smtp::Entity::MailBox mailBox;

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
