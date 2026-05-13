#include <openssl/ssl.h>
#include <LikesProgram/String.hpp>
#include <LikeHub/storage/database/DatabasePool.hpp>
#include <unordered_map>
#include <shared_mutex>
#include <memory>

namespace LikeHub {
    struct DkimKey {
        std::string privateKeyPem;
        std::string publicKeyPem;
    };
    struct DomainSecurityCredentials {
        std::string domainName;
        // TLS
        std::string tlsPrivateKey;
        std::string tlsCertificate;
        std::string caChain;

        // DKIM
        std::string dkimPrivateKey;
        std::string dkimPublicKey;
    };
    class SecurityContextManager {
    public:
        // 获取单例
        static SecurityContextManager& Instance();

        // 禁止拷贝
        SecurityContextManager(const SecurityContextManager&) = delete;
        SecurityContextManager& operator=(const SecurityContextManager&) = delete;

        // 禁止移动
        SecurityContextManager(SecurityContextManager&&) = delete;
        SecurityContextManager& operator=(SecurityContextManager&&) = delete;

        // 初始化
        static void Initialize(LikeHub::Storage::DatabasePool& pool);

        // TLS Context
        SSL_CTX* GetServerContext(const std::string& domain);

        // DKIM
        const DkimKey* GetDkimKey(const std::string& domain) const;

        // 重新加载
        bool UpdateDomain(const uint64_t domainId);
        // 移除
        void RemoveDomain(const std::string& domain);
    private:
        SecurityContextManager(LikeHub::Storage::DatabasePool& pool);
        using SSL_CTX_ptr = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>;
        SSL_CTX_ptr CreateContext(const DomainSecurityCredentials& creds);
    private:
        static std::unique_ptr<SecurityContextManager> s_instance;
        static std::once_flag s_initFlag;
        LikeHub::Storage::DatabasePool& m_dbPool;

        std::unordered_map<std::string, SSL_CTX_ptr> m_tlsContexts;
        std::unordered_map<std::string, DkimKey> m_dkimKeys;
        mutable std::shared_mutex m_mutex;
    };
}