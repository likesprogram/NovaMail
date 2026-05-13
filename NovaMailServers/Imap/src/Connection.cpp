#include <Connection.hpp>
#include <Session.hpp>

namespace NovaMail {
    namespace Imap {
        Connection::Connection(SocketType fd, LikesProgram::Net::EventLoop* loop, std::unique_ptr<LikesProgram::Net::Transport> transport)
        : LikesProgram::Net::Connection(fd, loop, std::move(transport)), m_session(std::make_shared<Session>(this, GetRemoteAddress(), GetLocalAddress())) { }
        Connection::~Connection() { }

        void Connection::OnConnected() {
            m_session->OnConnect();
        }

        void Connection::OnHandshakeDone() {
            if (m_session) m_session->OnTlsHandshakeDone();
        }

        void Connection::OnMessage(LikesProgram::Net::Buffer& in) {
            if (!m_session) {
                Shutdown();   // 优雅关闭
                return;
            }
            m_session->OnBytes(in);
            if (m_session && m_session->ShouldClose()) Shutdown();   // 优雅关闭
        }
    }
}