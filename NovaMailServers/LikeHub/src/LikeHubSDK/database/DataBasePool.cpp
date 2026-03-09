#include <LikeHub/database/DataBaseConnection.hpp>
#include <LikeHub/database/DataBasePool.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace LikeHub {
    std::unique_ptr<DataBasePool> DataBasePool::s_instance;
    std::once_flag DataBasePool::s_initFlag;

    void DataBasePool::Initialize(const std::string& uri, size_t poolSize) {
        if (s_instance) LogWarn(u"DataBasePool already initialized, reinitializing with new settings.");

        std::call_once(s_initFlag, [&](){
            LogInfo(u"Initializing DataBasePool, size={}", poolSize);
            s_instance.reset(new DataBasePool(uri, poolSize));
        });
    }

    DataBasePool& DataBasePool::Instance() {
        if (!s_instance) {
            LogFatal(u"DataBasePool not initialized. Call Initialize() first.");
            throw std::runtime_error("DataBasePool not initialized");
        }

        return *s_instance;
    }

    DataBasePool::DataBasePool(const std::string& uri, size_t poolSize) : m_uri(uri) {
        LogDebug(u"Creating MySQL connection pool...");

        for(size_t i = 0; i < poolSize; i++) {
            try {
                m_pool.push({new mysqlx::Session(uri), std::chrono::steady_clock::now()});
                LogDebug(u"MySQL connection created [{}/{}]", i + 1, poolSize);
            }
            catch(const std::exception& e) {
                LogError(u"DataBasePool connection create failed: {}", e.what());
            }
        }

        if (m_pool.empty())
        {
            LogFatal(u"No MySQL connections created. Database unavailable.");
            throw std::runtime_error("Database pool initialization failed");
        }
    
        LogDebug(u"DataBasePool initialized, available={}", m_pool.size());
    }

    DataBasePool::~DataBasePool() {
        LogDebug(u"Destroying DataBasePool...");

        while (!m_pool.empty()) {
            mysqlx::Session* session = m_pool.front().session;
            m_pool.pop();
            delete session;
        }

        LogDebug(u"DataBasePool destroyed");
    }

    std::shared_ptr<DataBaseConnection> DataBasePool::acquire() {
        std::unique_lock<std::mutex> lock(m_mutex);

        while(m_pool.empty()) {
            LogWarn(u"DataBasePool exhausted, waiting for connection...");
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

        LogDebug(u"DataBasePool acquire connection, remaining={}", m_pool.size());
        return std::shared_ptr<DataBaseConnection>(new DataBaseConnection(entry.session, this));
    }

    void DataBasePool::release(mysqlx::Session* session) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_pool.push({session, std::chrono::steady_clock::now()});

        LogDebug(u"DataBasePool release connection, available={}", m_pool.size());

        m_cond.notify_one();
    }
}