#include <service/UserService.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Smtp::Service {
        // 从 数据库 行 转换成 User 对象
        static bool RowToUser(mysqlx::Row row, Entity::User& user) {
            if (!row) return false;

            if (!row[0].isNull()) user.userId    = row[0].get<uint64_t>();
            if (!row[1].isNull()) user.account   = row[1].get<std::string>();
            if (!row[2].isNull()) user.nickname  = row[2].get<std::string>();
            if (!row[3].isNull()) user.isAdmin   = row[3].get<int>() == 1;
            if (!row[4].isNull()) user.status    = UserService::ParseUserStatus(row[4].get<std::string>());

            return true;
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
                    "SELECT u.userId, u.account, p.nickname, u.isAdmin, u.status "
                    "FROM Users u "
                    "JOIN UserProfile p ON u.userId = p.userId "
                    "JOIN EmailAuthorization ea ON u.userId = ea.userId "
                    "WHERE u.account = ? AND ea.authCode = ? "
                    "AND ea.isActivated = 1 AND (ea.expireTime IS NULL OR ea.expireTime > NOW()) "
                    "AND u.status = 'NORMAL' LIMIT 1;"
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
                    "SELECT u.userId, u.account, p.nickname, u.isAdmin, u.status "
                    "FROM MailBoxes mb JOIN Users u ON u.userId = mb.userId "
                    "JOIN UserProfile p ON p.userId = u.userId "
                    "JOIN DomainNames dn ON dn.domainNameId = mb.domainNameId "
                    "JOIN EmailAuthorization ea ON u.userId = ea.userId "
                    "WHERE mb.localPart = ? AND dn.domainName = ? AND ea.authCode = ? "
                    "AND ea.isActivated = 1 AND (ea.expireTime IS NULL OR ea.expireTime > NOW()) "
                    "AND mb.isOpen = 1 AND u.status = 'NORMAL' LIMIT 1;"
                ).bind(std::string(localPart)).bind(std::string(domain)).bind(std::string(authCode)).execute();

                return RowToUser(result.fetchOne(), user);
            } catch (const std::exception& e) {
                LogError(u"GetUserByMailboxAuthCode failed: localPart={}, domain={}, error={}", std::string(localPart), std::string(domain), e.what());
                return false;
            }
        }
    }
}
