#pragma once
#include <LikesProgram/net/Connection.hpp>
#include <Session.hpp>

namespace NovaMail {
    namespace Imap {
        class Connection final : public LikesProgram::Net::Connection {
        public:
            Connection(SocketType fd, LikesProgram::Net::EventLoop* loop, std::unique_ptr<LikesProgram::Net::Transport> transport);
            ~Connection();

            // 连接建立完成
            void OnConnected() override;

            // 握手完成
            void OnHandshakeDone() override;

            // 收到数据
            void OnMessage(LikesProgram::Net::Buffer& in) override;
        private:
            std::shared_ptr<Session> m_session;
        };
    }
}
