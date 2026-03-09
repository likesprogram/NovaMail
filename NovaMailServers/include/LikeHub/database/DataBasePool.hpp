#pragma once
#include <mysqlx/xdevapi.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace LikeHub {
    class DataBaseConnection;
    
    class DataBasePool
    {
    public:
        // 获取单例
        static DataBasePool& Instance();

        // 初始化连接池
        static void Initialize(const std::string& uri, size_t poolSize);

        // 获取连接
        std::shared_ptr<DataBaseConnection> acquire();

        // 归还连接
        void release(mysqlx::Session* session);
        
        ~DataBasePool();
    private:
        // 构造函数私有化
        DataBasePool(const std::string& uri, size_t poolSize);

        // 禁止拷贝
        DataBasePool(const DataBasePool&) = delete;
        DataBasePool& operator=(const DataBasePool&) = delete;
        
    private:
        static std::unique_ptr<DataBasePool> s_instance;
        static std::once_flag s_initFlag;

        struct PoolEntry {
            mysqlx::Session* session;
            std::chrono::steady_clock::time_point lastUsed;
        };
        std::queue<PoolEntry> m_pool;
    
        std::mutex m_mutex;
        std::condition_variable m_cond;
    
        std::string m_uri;
    };
}