#pragma once
#include <LikeHub/RpcMessage.hpp>
#include <memory>

namespace LikeHub {
    class Session;
    class HandleDispatcher {
    public:
        // 请求处理函数
        static void HandleRequest(const std::shared_ptr<Session>& session, const RpcMessage& request);
        // 回调处理函数
        static void HandleResponse(const std::shared_ptr<Session>& session, const RpcMessage& response, const  RpcMessage& request, double delayMs);
        // 未知请求处理函数
        static void HandleUnknown(const std::shared_ptr<Session>& session, const RpcMessage& message);
        // 请求超时处理函数
        static void HandleTimeout(const std::shared_ptr<Session>& session, const RpcMessage& request);
    };
}
