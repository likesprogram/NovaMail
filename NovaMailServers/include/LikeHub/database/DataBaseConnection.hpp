#pragma once
#include <mysqlx/xdevapi.h>
#include <memory>

namespace LikeHub {
    class DataBasePool;

    class DataBaseConnection
    {
    public:
        DataBaseConnection(mysqlx::Session* session, DataBasePool* pool);
        ~DataBaseConnection();

        mysqlx::Session* operator->();
        mysqlx::Session& operator*();

    private:
        mysqlx::Session* m_session;
        DataBasePool* m_pool;
    };
}
