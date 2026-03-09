#include <Connection.hpp>

namespace NovaMail {
    namespace Smtp {
        Connection::Connection(SocketType fd, LikesProgram::Net::EventLoop* loop, std::unique_ptr<LikesProgram::Net::Transport> transport)
        : LikesProgram::Net::Connection(fd, loop, std::move(transport)) { }
        Connection::~Connection() { }

        void Connection::OnConnected() {
            LikesProgram::Net::Buffer out;
            m_session.OnConnect(out);
            Send(out);
        }

        void Connection::OnMessage(LikesProgram::Net::Buffer& in) {
            LikesProgram::Net::Buffer out;
            m_session.OnBytes(in, out);
            if (out.ReadableBytes() > 0) Send(out);
            if (m_session.ShouldClose()) Shutdown();   // 优雅关闭
        }
    }
}