#include <Connection.hpp>
#include <LikeHub/RpcMessage.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace LikeHub {
    Connection::Connection(SocketType fd, LikesProgram::Net::EventLoop* loop, std::unique_ptr<LikesProgram::Net::Transport> transport)
    : LikesProgram::Net::Connection(fd, loop, std::move(transport)), m_session(std::make_shared<Session>(this, GetRemoteAddress(), GetLocalAddress())) { }
    Connection::~Connection() {
        if (m_session) m_session.reset();
    }

    void Connection::OnConnected() {
        if (!m_session) return;
        m_session->OnConnect();
    }
    
    void Connection::OnMessage(LikesProgram::Net::Buffer& in) {
        if (!m_session) {
            Shutdown();   // 优雅关闭
            return;
        }
        for (;;) {
            // 解析指令
            RpcMessage::ParseResult result = RpcMessage::TryParse(in, 1024 * 1024 *1024);
            // 判断解析结果
            if (result.status == RpcMessage::ParseStatus::Success) {
                if (!result.message) {
                    LogError(u"Rpc parse success but message is null from {}", GetRemoteAddress().ToString());
                    m_session->ShouldClose(true, true);
                    break;
                }
                m_session->OnMessage(*result.message); // 解析成功，处理消息
                if (m_session->ShouldClose()) break;
                continue; // 开启下一次循环
            }
            else if (result.status == RpcMessage::ParseStatus::Incomplete) return; // 等待更多数据
            else if (result.status == RpcMessage::ParseStatus::InvalidMagic) {
                LogError(u"Invalid magic from {}: {}", GetRemoteAddress().ToString(), result.errorMsg);
                m_session->ShouldClose(true, true); // 魔数错误，关闭连接
                break;
            } else if (result.status == RpcMessage::ParseStatus::MetaLengthTooLarge || result.status == RpcMessage::ParseStatus::BodyLengthTooLarge) {
                LogError(u"Rpc message too large from {}: {}", GetRemoteAddress().ToString(), result.errorMsg );
                m_session->ShouldClose(true, true);
                break;
            } else {
                LogError(u"Failed to parse message from {}: {}", GetRemoteAddress().ToString(), result.errorMsg);
                LikesProgram::Configuration metadata; // 构造错误响应元数据
                metadata["message"] = LikesProgram::String(result.errorMsg);
                m_session->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, 0, RpcMessage::Code::BadRequest, metadata, 0)); // 发送错误响应
                break;
            }
        }
        // 是否需要关闭连接
        if (m_session && m_session->ShouldClose()) Shutdown();   // 优雅关闭
    }
}