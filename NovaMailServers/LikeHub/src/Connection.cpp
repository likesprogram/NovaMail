#include <Connection.hpp>

namespace LikeHub {
    Connection::Connection(SocketType fd, LikesProgram::Net::EventLoop* loop, std::unique_ptr<LikesProgram::Net::Transport> transport)
    : LikesProgram::Net::Connection(fd, loop, std::move(transport)) {
        instance = new ServiceInstance();
    }
    Connection::~Connection() {
        
        delete instance;
        instance = nullptr;
    }
    void Connection::OnConnected() { }
    
    void Connection::OnMessage(LikesProgram::Net::Buffer& in) {
        (void)in;
    }
}