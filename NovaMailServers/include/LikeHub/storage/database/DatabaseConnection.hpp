#pragma once
#include <mysqlx/xdevapi.h>
#include <memory>

namespace LikeHub {
    namespace Storage {
        class DatabasePool;
        class DatabaseConnection {
        public:
            DatabaseConnection(mysqlx::Session* session, DatabasePool* pool);
            ~DatabaseConnection();

            mysqlx::Session* operator->();
            mysqlx::Session& operator*();

        private:
            mysqlx::Session* m_session;
            DatabasePool* m_pool;
        };
    }
}
