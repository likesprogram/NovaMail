#include <LikeHub/storage/database/DatabaseConnection.hpp>
#include <LikeHub/storage/database/DatabasePool.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace LikeHub {
    namespace Storage {
        DatabaseConnection::DatabaseConnection(mysqlx::Session* session, DatabasePool* pool) : m_session(session), m_pool(pool) {
            LogDebug(u"DatabaseConnection created, session={}", (void*)m_session);
        }
        
        DatabaseConnection::~DatabaseConnection() {
            if (m_session) {
                LogDebug(u"DatabaseConnection releasing session back to pool, session={}", (void*)m_session);
                m_pool->Release(m_session);
                m_session = nullptr;
            } else {
                LogWarn(u"DatabaseConnection destructor called with null session.");
            }
        }
        
        mysqlx::Session* DatabaseConnection::operator->() {
            if (!m_session) {
                LogError(u"DatabaseConnection operator-> called with null session.");
                return nullptr;
            }
            return m_session;
        }
        
        mysqlx::Session& DatabaseConnection::operator*() {
            if (!m_session) {
                LogError(u"DatabaseConnection operator* called with null session.");
                throw std::runtime_error("Attempted to use a released Database connection.");
            }
            return *m_session;
        }
    }
}
