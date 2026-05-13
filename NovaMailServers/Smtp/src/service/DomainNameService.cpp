#include <service/DomainNameService.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Smtp::Service {
        DomainNameService::DomainNameService(LikeHub::Storage::DatabasePool& pool): IService(pool) { }

        std::string DomainNameService::GetPrivateKey(uint64_t domainNameId) {
            if (domainNameId == 0) return "";
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(u"GetPrivateKey failed: database connection unavailable");
                return "";
            }
        
            try {
                auto result = (*conn)->sql(
                    "SELECT dc.sigPrivateKey FROM DomainNames dn "
                    "INNER JOIN DomainCredentials dc ON dc.domainNameId = dn.domainNameId "
                    "WHERE dn.domainNameId = ? AND dn.hasDKIM = 1 AND dc.sigPrivateKey IS NOT NULL AND dc.sigPrivateKey <> '' "
                    "LIMIT 1;"
                ).bind(domainNameId).execute();
            
                mysqlx::Row row = result.fetchOne();
                if (!row) {
                    LogWarn(u"GetPrivateKey failed: DKIM private key not found, domainNameId={}", domainNameId);
                    return "";
                }
            
                return row[0].get<std::string>();
            } catch (const std::exception& e) {
                LogError(u"GetPrivateKey failed: domainNameId={} error={}", domainNameId, e.what());
                return "";
            }
        }
    }
}
