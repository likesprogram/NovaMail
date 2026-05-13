#include <LikeHub/SecurityContextManager.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <LikeHub/storage/database/DatabaseConnection.hpp>
#include <openssl/err.h>
#include <optional>
#include <openssl/pem.h>
namespace LikeHub {
    static std::vector<DomainSecurityCredentials> GetAllSecurityDomains(LikeHub::Storage::DatabaseConnection& conn) {
        std::vector<DomainSecurityCredentials> resultList;
        try {
            auto result = conn->sql(
                "SELECT dn.domainName, dc.privateKey, dc.certificate, dc.caChain, dc.sigPrivateKey, dc.sigPublicKey "
                "FROM DomainNames dn LEFT JOIN DomainCredentials dc ON dn.domainNameId = dc.domainNameId "
                "WHERE dn.hasCertificate = 1 OR dn.hasDKIM = 1"
            ).execute();

            auto rows = result.fetchAll();

            for (const auto& row : rows) {
                DomainSecurityCredentials info;
                info.domainName = row[0].get<std::string>();

                // TLS
                if (!row[1].isNull()) info.tlsPrivateKey = row[1].get<std::string>();
                if (!row[2].isNull()) info.tlsCertificate = row[2].get<std::string>();
                if (!row[3].isNull()) info.caChain = row[3].get<std::string>();

                // DKIM
                if (!row[4].isNull()) info.dkimPrivateKey = row[4].get<std::string>();
                if (!row[5].isNull()) info.dkimPublicKey = row[5].get<std::string>();
                resultList.emplace_back(std::move(info));
            }
        } catch (const std::exception& e) {
            LogError(u"GetAllSecurityDomains failed: {}", e.what());
        }
        return resultList;
    }

    static std::optional<DomainSecurityCredentials> GetAllSecurityDomains(LikeHub::Storage::DatabaseConnection& conn, uint64_t domainId) {
        try {
            auto result = conn->sql(
                "SELECT dn.domainName, dc.privateKey, dc.certificate, dc.caChain, dc.sigPrivateKey, dc.sigPublicKey "
                "FROM DomainNames dn LEFT JOIN DomainCredentials dc ON dn.domainNameId = dc.domainNameId "
                "WHERE dn.domainNameId = ? LIMIT 1"
            ).bind(domainId).execute();

            auto row = result.fetchOne();
            if (row.isNull()) return std::nullopt;
            
            DomainSecurityCredentials info;
            info.domainName = row[0].get<std::string>();

            // TLS
            if (!row[1].isNull()) info.tlsPrivateKey = row[1].get<std::string>();
            if (!row[2].isNull()) info.tlsCertificate = row[2].get<std::string>();
            if (!row[3].isNull()) info.caChain = row[3].get<std::string>();

            // DKIM
            if (!row[4].isNull()) info.dkimPrivateKey = row[4].get<std::string>();
            if (!row[5].isNull()) info.dkimPublicKey = row[5].get<std::string>();
            return info;
        } catch (const std::exception& e) {
            LogError(u"GetSecurityDomainById failed: {}", e.what());
        }
        return std::nullopt;
    }

    std::unique_ptr<SecurityContextManager> SecurityContextManager::s_instance;
    std::once_flag SecurityContextManager::s_initFlag;

    void SecurityContextManager::Initialize(LikeHub::Storage::DatabasePool& pool) {
        if (s_instance) LogWarn(u"SecurityContextManager already initialized, reinitializing with new settings.");
        std::call_once(s_initFlag, [&](){
            LogInfo(u"Initializing SecurityContextManager");
            s_instance.reset(new SecurityContextManager(pool));
        });
    }

    SecurityContextManager::SecurityContextManager(LikeHub::Storage::DatabasePool& pool) : m_dbPool(pool) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
        auto conn = pool.Acquire();

        if (!conn) {
            LogError(u"GetAllSecurityDomains failed: database connection unavailable");
        } else {
            auto domains = GetAllSecurityDomains((*conn));
            for (auto& domain : domains) {
                auto ctx = CreateContext(domain);
                if (ctx) m_tlsContexts.insert_or_assign(domain.domainName, std::move(ctx));
                DkimKey dkim;
                dkim.privateKeyPem = domain.dkimPrivateKey;
                dkim.publicKeyPem = domain.dkimPublicKey;
                m_dkimKeys.insert_or_assign(domain.domainName, std::move(dkim));
            }
        }
    }
    
    SecurityContextManager& SecurityContextManager::Instance() {
        if (!s_instance) {
            LogFatal(u"SecurityContextManager not initialized. Call Initialize() first.");
            throw std::runtime_error("SecurityContextManager not initialized");
        }

        return *s_instance;
    }

    SSL_CTX* SecurityContextManager::GetServerContext(const std::string& domain) {
        std::shared_lock lock(m_mutex);
        auto it = m_tlsContexts.find(domain);
        if (it == m_tlsContexts.end()) return nullptr;
        return it->second.get();
    }

    const DkimKey* SecurityContextManager::GetDkimKey(const std::string& domain) const {
        std::shared_lock lock(m_mutex);
        auto it = m_dkimKeys.find(domain);
        if (it == m_dkimKeys.end()) return nullptr;
        return &it->second;
    }

    bool SecurityContextManager::UpdateDomain(uint64_t domainId) {
        auto conn = m_dbPool.Acquire();

        if (!conn) {
            LogError(u"UpdateDomain failed: database connection unavailable");
            return false;
        }

        auto domainSecurityCredentials = GetAllSecurityDomains((*conn), domainId);

        if (!domainSecurityCredentials.has_value()) return false;

        auto ctx = CreateContext(*domainSecurityCredentials);

        if (!ctx) return false;

        DkimKey dkim;
        dkim.privateKeyPem = domainSecurityCredentials->dkimPrivateKey;
        dkim.publicKeyPem = domainSecurityCredentials->dkimPublicKey;

        {
            std::unique_lock lock(m_mutex);
            m_tlsContexts.insert_or_assign(domainSecurityCredentials->domainName, std::move(ctx));
            m_dkimKeys.insert_or_assign(domainSecurityCredentials->domainName, std::move(dkim));
        }

        return true;
    }

    void SecurityContextManager::RemoveDomain(const std::string& domain) {
        std::unique_lock lock(m_mutex);
        m_tlsContexts.erase(domain);
        m_dkimKeys.erase(domain);
    }

    SecurityContextManager::SSL_CTX_ptr SecurityContextManager::CreateContext(const DomainSecurityCredentials& creds) {
        SSL_CTX* rawCtx = SSL_CTX_new(TLS_server_method());
        if (!rawCtx) return SSL_CTX_ptr(nullptr, SSL_CTX_free);
        SSL_CTX_ptr ctx(rawCtx, SSL_CTX_free);
        SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION);

        //  安全选项
        SSL_CTX_set_options(ctx.get(), SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);

        // 加载证书
        int chainCount = 0;
        BIO* certBio = BIO_new_mem_buf(creds.tlsCertificate.data(), static_cast<int>(creds.tlsCertificate.size()));
        if (!certBio) return SSL_CTX_ptr(nullptr, SSL_CTX_free);
        X509* cert = PEM_read_bio_X509(certBio, nullptr, nullptr, nullptr);
        if (!cert) {
            BIO_free(certBio);
            return SSL_CTX_ptr(nullptr, SSL_CTX_free);
        }
        if (SSL_CTX_use_certificate(ctx.get(), cert) != 1) {
            X509_free(cert);
            BIO_free(certBio);
            return SSL_CTX_ptr(nullptr, SSL_CTX_free);
        }
        X509_free(cert);
        // 继续从 tlsCertificate 中读取后续证书，应对 tlsCertificate 是 fullchain.pem 的情况
        while (true) {
            X509* chainCert = PEM_read_bio_X509(certBio, nullptr, nullptr, nullptr);
            if (!chainCert) break;
            if (SSL_CTX_add_extra_chain_cert(ctx.get(), chainCert) != 1) {
                X509_free(chainCert);
                BIO_free(certBio);
                return SSL_CTX_ptr(nullptr, SSL_CTX_free);
            }
            // SSL_CTX_add_extra_chain_cert 成功后，SSL_CTX 接管 chainCert 所有权，不能再 X509_free
            ++chainCount;
        }
        BIO_free(certBio);

        // 加载私钥
        BIO* keyBio = BIO_new_mem_buf(creds.tlsPrivateKey.data(), static_cast<int>(creds.tlsPrivateKey.size()));
        if (!keyBio) return SSL_CTX_ptr(nullptr, SSL_CTX_free);
        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(keyBio, nullptr, nullptr, nullptr);
        BIO_free(keyBio);
        if (!pkey) return SSL_CTX_ptr(nullptr, SSL_CTX_free);
        if (SSL_CTX_use_PrivateKey(ctx.get(), pkey) != 1) {
            EVP_PKEY_free(pkey);
            return SSL_CTX_ptr(nullptr, SSL_CTX_free);
        }
        EVP_PKEY_free(pkey);

        // 验证私钥和证书是否匹配
        if (SSL_CTX_check_private_key(ctx.get()) != 1) return SSL_CTX_ptr(nullptr, SSL_CTX_free);

        // 额外加载 caChain 应对 tlsCertificate = cert.pem，caChain = chain.pem
        if (!creds.caChain.empty()) {
            BIO* chainBio = BIO_new_mem_buf(creds.caChain.data(), static_cast<int>(creds.caChain.size()));
            if (chainBio) {
                while (true) {
                    X509* ca = PEM_read_bio_X509(chainBio, nullptr, nullptr, nullptr);
                    if (!ca) break;
                    if (SSL_CTX_add_extra_chain_cert(ctx.get(), ca) != 1) {
                        X509_free(ca);
                        BIO_free(chainBio);
                        return SSL_CTX_ptr(nullptr, SSL_CTX_free);
                    }
                    // 成功后 SSL_CTX 接管 ca
                    ++chainCount;
                }
                BIO_free(chainBio);
            }
        }

        // 会话缓存
        SSL_CTX_set_session_cache_mode(ctx.get(), SSL_SESS_CACHE_SERVER);
        return ctx;
    }

}