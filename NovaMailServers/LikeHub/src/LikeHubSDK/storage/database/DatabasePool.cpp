#include <LikeHub/storage/database/DatabaseConnection.hpp>
#include <LikeHub/storage/database/DatabasePool.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace LikeHub {
    namespace Storage {
        std::unique_ptr<DatabasePool> DatabasePool::s_instance;
        std::once_flag DatabasePool::s_initFlag;

        void DatabasePool::Initialize(const std::string& uri, size_t poolSize) {
            if (s_instance) LogWarn(u"DatabasePool already initialized, reinitializing with new settings.");

            std::call_once(s_initFlag, [&](){
                LogInfo(u"Initializing DatabasePool, size={}", poolSize);
                s_instance.reset(new DatabasePool(uri, poolSize));
            });
        }

        DatabasePool& DatabasePool::Instance() {
            if (!s_instance) {
                LogFatal(u"DatabasePool not initialized. Call Initialize() first.");
                throw std::runtime_error("DatabasePool not initialized");
            }

            return *s_instance;
        }

        DatabasePool::DatabasePool(const std::string& uri, size_t poolSize) : m_uri(uri) {
            LogDebug(u"Creating MySQL connection pool...");

            for(size_t i = 0; i < poolSize; i++) {
                try {
                    m_pool.push({new mysqlx::Session(uri), std::chrono::steady_clock::now()});
                    LogDebug(u"MySQL connection created [{}/{}]", i + 1, poolSize);
                }
                catch(const std::exception& e) {
                    LogError(u"DatabasePool connection create failed: {}", e.what());
                }
            }

            if (m_pool.empty())
            {
                LogFatal(u"No MySQL connections created. Database unavailable.");
                throw std::runtime_error("Database pool initialization failed");
            }
        
            LogDebug(u"DatabasePool initialized, available={}", m_pool.size());
        }

        DatabasePool::~DatabasePool() {
            LogDebug(u"Destroying DatabasePool...");

            while (!m_pool.empty()) {
                mysqlx::Session* session = m_pool.front().session;
                m_pool.pop();
                delete session;
            }

            LogDebug(u"DatabasePool destroyed");
        }

        std::shared_ptr<DatabaseConnection> DatabasePool::Acquire() {
            std::unique_lock<std::mutex> lock(m_mutex);

            while(m_pool.empty()) {
                LogWarn(u"DatabasePool exhausted, waiting for connection...");
                m_cond.wait(lock);
            }

            PoolEntry entry = m_pool.front();
            m_pool.pop();

            lock.unlock();

            // 连接健康检查
            auto now = std::chrono::steady_clock::now();
            if (now - entry.lastUsed > std::chrono::seconds(60)) {
                try { entry.session->sql("DO 1").execute(); }
                catch (...) {
                    LogWarn(u"MySQL connection lost, reconnecting");
                    delete entry.session;
                    entry.session = new mysqlx::Session(m_uri);
                }
            }

            LogDebug(u"DatabasePool acquire connection, remaining={}", m_pool.size());
            return std::shared_ptr<DatabaseConnection>(new DatabaseConnection(entry.session, this));
        }

        void DatabasePool::Release(mysqlx::Session* session) {
            std::lock_guard<std::mutex> lock(m_mutex);

            m_pool.push({session, std::chrono::steady_clock::now()});

            LogDebug(u"DatabasePool release connection, available={}", m_pool.size());

            m_cond.notify_one();
        }
    }
}