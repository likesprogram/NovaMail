#include <service/UserService.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Imap::Service {
        // 从 数据库 行 转换成 User 对象
        static bool RowToUser(mysqlx::Row row, Entity::User& user) {
            if (!row) return false;

            if (!row[0].isNull()) user.userId    = row[0].get<uint64_t>();
            if (!row[1].isNull()) user.account   = row[1].get<std::string>();
            if (!row[2].isNull()) user.nickname  = row[2].get<std::string>();
            if (!row[3].isNull()) user.isAdmin   = row[3].get<int>() == 1;
            if (!row[4].isNull()) user.status    = UserService::ParseUserStatus(row[4].get<std::string>());
            if (!row[5].isNull()) user.mailBoxId = row[5].get<uint64_t>();
            if (!row[6].isNull()) user.localPart = row[6].get<std::string>();
            if (!row[7].isNull()) user.domainName = row[7].get<std::string>();
            if (!user.localPart.empty() && !user.domainName.empty()) user.mailAddress = user.localPart + "@" + user.domainName;
        
            return user.Valid() && user.HasMailBox();
        }

        UserService::UserService(LikeHub::Storage::DatabasePool& pool): IService(pool) { }

        Entity::UserStatus UserService::ParseUserStatus(std::string_view s) {
            if (s == "NORMAL")   return Entity::UserStatus::Normal;
            if (s == "DISABLED") return Entity::UserStatus::Disabled;
            if (s == "DELETED")  return Entity::UserStatus::Deleted;
            return Entity::UserStatus::Normal;
        }

        bool UserService::GetUser(uint64_t userId, Entity::User& user) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return false;
            try {
                auto result = (*conn)->sql(
                    "SELECT u.userId, u.account, p.nickname, u.isAdmin, u.status "
                    "FROM Users u "
                    "JOIN UserProfile p ON u.userId = p.userId "
                    "WHERE u.userId = ? "
                    "LIMIT 1;"
                ).bind(userId).execute();

                return RowToUser(result.fetchOne(), user);
            } catch (...) {
                return false;
            }
        }

        bool UserService::GetUserByAccountAuthCode(std::string_view account, std::string_view authCode, Entity::User& user) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return false;
            try {
                auto result = (*conn)->sql(
                    "SELECT "
                    "    u.userId, "
                    "    u.account, "
                    "    p.nickname, "
                    "    u.isAdmin, "
                    "    u.status, "
                    "    COALESCE(dmb.mailBoxId, fmb.mailBoxId) AS mailBoxId, "
                    "    COALESCE(dmb.localPart, fmb.localPart) AS localPart, "
                    "    COALESCE(ddn.domainName, fdn.domainName) AS domainName "
                    "FROM Users u "
                    "JOIN UserProfile p ON u.userId = p.userId "
                    "JOIN EmailAuthorization ea ON u.userId = ea.userId "

                    // 默认邮箱，必须属于当前用户且启用
                    "LEFT JOIN MailBoxes dmb "
                    "    ON dmb.mailBoxId = p.defaultMailBoxId "
                    "   AND dmb.userId = u.userId "
                    "   AND dmb.isOpen = 1 "
                    "LEFT JOIN DomainNames ddn "
                    "    ON ddn.domainNameId = dmb.domainNameId "

                    // 默认邮箱不可用时，取第一个启用邮箱
                    "LEFT JOIN MailBoxes fmb "
                    "    ON fmb.mailBoxId = ( "
                    "        SELECT mb2.mailBoxId "
                    "        FROM MailBoxes mb2 "
                    "        WHERE mb2.userId = u.userId "
                    "          AND mb2.isOpen = 1 "
                    "        ORDER BY mb2.mailBoxId ASC "
                    "        LIMIT 1 "
                    "    ) "
                    "LEFT JOIN DomainNames fdn "
                    "    ON fdn.domainNameId = fmb.domainNameId "

                    "WHERE u.account = ? "
                    "  AND ea.authCode = ? "
                    "  AND ea.isActivated = 1 "
                    "  AND (ea.expireTime IS NULL OR ea.expireTime > NOW()) "
                    "  AND u.status = 'NORMAL' "
                    "  AND (dmb.mailBoxId IS NOT NULL OR fmb.mailBoxId IS NOT NULL) "
                    "LIMIT 1;"
                ).bind(std::string(account)).bind(std::string(authCode)).execute();

                return RowToUser(result.fetchOne(), user);
            } catch (const std::exception& e) {
                LogError(u"InsertMail failed: localPart={}, {}", std::string(account), e.what());
                return false;
            }
        }

        bool UserService::GetUserByMailboxAuthCode(std::string_view localPart, std::string_view domain, std::string_view authCode, Entity::User& user) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return false;
            try {
                auto result = (*conn)->sql(
                    "SELECT "
                    "    u.userId, "
                    "    u.account, "
                    "    p.nickname, "
                    "    u.isAdmin, "
                    "    u.status, "
                    "    mb.mailBoxId, "
                    "    mb.localPart, "
                    "    dn.domainName "
                    "FROM MailBoxes mb "
                    "JOIN Users u ON u.userId = mb.userId "
                    "JOIN UserProfile p ON p.userId = u.userId "
                    "JOIN DomainNames dn ON dn.domainNameId = mb.domainNameId "
                    "JOIN EmailAuthorization ea ON u.userId = ea.userId "
                    "WHERE mb.localPart = ? "
                    "  AND dn.domainName = ? "
                    "  AND ea.authCode = ? "
                    "  AND ea.isActivated = 1 "
                    "  AND (ea.expireTime IS NULL OR ea.expireTime > NOW()) "
                    "  AND mb.isOpen = 1 "
                    "  AND u.status = 'NORMAL' "
                    "LIMIT 1;"
                ).bind(std::string(localPart)).bind(std::string(domain)).bind(std::string(authCode)).execute();

                return RowToUser(result.fetchOne(), user);
            } catch (const std::exception& e) {
                LogError(u"GetUserByMailboxAuthCode failed: localPart={}, domain={}, error={}", std::string(localPart), std::string(domain), e.what());
                return false;
            }
        }
    }
}
