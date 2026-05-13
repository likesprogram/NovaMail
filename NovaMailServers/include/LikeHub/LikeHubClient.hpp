#pragma once
#include <LikesProgram/net/Client.hpp>
#include <LikesProgram/String.hpp>
#include <LikeHub/EventListener.hpp>
#include <LikesProgram/time/Timer.hpp>
#include <LikeHub/DeadlineScheduler.hpp>
#include <string>
#include <mutex>
#include <optional>
#include <atomic>
#include <cstdint>
#include <shared_mutex>
#include <memory>

namespace LikeHub {
    class LikeHubClient : public std::enable_shared_from_this<LikeHubClient> {
    public:
        // 获取单例
        static LikeHubClient& Instance();
        // 禁止拷贝
        LikeHubClient(const LikeHubClient&) = delete;
        LikeHubClient& operator=(const LikeHubClient&) = delete;

        // 禁止移动
        LikeHubClient(LikeHubClient&&) = delete;
        LikeHubClient& operator=(LikeHubClient&&) = delete;

        // 初始化
        static void Initialize(
            const LikesProgram::Net::Address& remoteAddr, const LikesProgram::String& serviceName,
            const LikesProgram::String& access, const LikesProgram::String& secret,
            std::function<void()> loginSuccess,  std::function<void(std::string)> loginFailure
        ) ;
        LikesProgram::Net::Client& GetClient();
        // 消息派发
        void OnMessage(const RpcMessage& message);
        // 发送请求
        void SendMessage(const RpcMessage& message, size_t retryCount = 0);
        // 事件注册
        void RegisterEvent(const LikesProgram::String& eventName, EventHandler handler, ListenType type);
        void NotifyConnectionClosed(const std::string& reason);
        void NotifyConnectionError(const std::string& reason);
    public:
        ~LikeHubClient();
    private:
        LikeHubClient(
            const LikesProgram::Net::Address& remoteAddr, const LikesProgram::String& serviceName,
            const LikesProgram::String& access, const LikesProgram::String& secret,
            std::function<void()> loginSuccess,  std::function<void(std::string)> loginFailure
        );
        
        // 发送登录请求
        void SendLoginRequest();
        // 登录返回
        void LoginResponse(const RpcMessage& response);
        // 通知失败一次
        void NotifyFailureOnce(const std::string& message);

        // 获取服务信息事件
        void OnGetServiceInfo(const RpcMessage& message);

        // 心跳事件
        void OnHeartbeat(uint64_t requestId);

        // 请求响应超时
        void OnRequestTimeout(uint64_t requestId);

        // 构建请求ID
        uint64_t RequestId();
    private:
        static std::shared_ptr<LikeHubClient> s_instance;
        static std::once_flag s_initFlag;
        LikesProgram::Net::Client m_client;   // 客户端实例
        LikesProgram::String m_serviceName;   // 服务名称
        uint64_t m_serviceId;                 // 服务ID
        LikesProgram::String m_access;        // 账号
        LikesProgram::String m_secret;        // 授权码

        std::function<void()> m_loginSuccessCallback;
        std::function<void(std::string)> m_loginFailureCallback;
        std::atomic_bool m_loginSuccess{false};
        std::atomic_bool m_failureNotified{false};

        int64_t m_timeoutMilliseconds = 1000 * 10;         // 请求超时时间（毫秒）
        int64_t m_requestRetryCount = 3;                 // 请求重试次数

        uint32_t m_communicationCounter;                // 通信计数器
        // 代返回请求（发送了请求，但是客户端还没返回）
        struct PendingRequest {
            RpcMessage message;
            size_t retryCount = 0; // 重试次数
            LikesProgram::Time::Timer timer; // 计时器 (用于计算延迟)
            DeadlineScheduler::TimerId timerId; // 超时定时器ID
        };
        std::unordered_map<uint64_t, PendingRequest> m_pending;

        std::unordered_map<std::string, EventHandler> m_events; // 事件名称 -> 监听桶
        mutable std::mutex m_eventsMutex;
    };
}