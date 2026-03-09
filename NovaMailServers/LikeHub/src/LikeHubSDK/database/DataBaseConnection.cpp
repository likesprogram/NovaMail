#include <LikeHub/database/DataBaseConnection.hpp>
#include <LikeHub/database/DataBasePool.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace LikeHub {
    DataBaseConnection::DataBaseConnection(mysqlx::Session* session, DataBasePool* pool) : m_session(session), m_pool(pool) {
        LogDebug("DataBaseConnection created, session={}", (void*)m_session);
    }
    
    DataBaseConnection::~DataBaseConnection() {
        if (m_session) {
            LogDebug(u"DataBaseConnection releasing session back to pool, session={}", (void*)m_session);
            m_pool->release(m_session);
            m_session = nullptr;
        } else {
            LogWarn(u"DataBaseConnection destructor called with null session.");
        }
    }
    
    mysqlx::Session* DataBaseConnection::operator->() {
        if (!m_session) {
            LogError(u"DataBaseConnection operator-> called with null session.");
            return nullptr;
        }
        return m_session;
    }
    
    mysqlx::Session& DataBaseConnection::operator*() {
        if (!m_session) {
            LogError(u"DataBaseConnection operator* called with null session.");
            throw std::runtime_error("Attempted to use a released database connection.");
        }
        return *m_session;
    }
}