#pragma once
#include <mysqlx/xdevapi.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace LikeHub {
    namespace Storage {
        class DatabaseConnection;
        
        class DatabasePool {
        public:
            // 获取单例
            static DatabasePool& Instance();
            
            // 禁止拷贝
            DatabasePool(const DatabasePool&) = delete;
            DatabasePool& operator=(const DatabasePool&) = delete;

            // 禁止移动
            DatabasePool(DatabasePool&&) = delete;
            DatabasePool& operator=(DatabasePool&&) = delete;

            // 初始化连接池
            static void Initialize(const std::string& uri, size_t poolSize);

            // 获取连接
            std::shared_ptr<DatabaseConnection> Acquire();

            // 归还连接
            void Release(mysqlx::Session* session);
            
            ~DatabasePool();
        private:
            // 构造函数私有化
            DatabasePool(const std::string& uri, size_t poolSize);
        private:
            static std::unique_ptr<DatabasePool> s_instance;
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
}