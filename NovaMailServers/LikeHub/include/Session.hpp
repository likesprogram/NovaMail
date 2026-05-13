#pragma once
#include <LikeHub/DeadlineScheduler.hpp>
#include <LikesProgram/Configuration.hpp>
#include <LikesProgram/net/Address.hpp>
#include <LikesProgram/time/Timer.hpp>
#include <LikesProgram/String.hpp>
#include <Service.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <atomic>
#include <cstdint>

namespace LikeHub {
    class Connection;
    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(Connection* connection, LikesProgram::Net::Address remoteAddr, LikesProgram::Net::Address localAddr);
        ~Session();

        void OnConnect();
        void OnMessage(const RpcMessage& message); // 收到指令, 派发处理
        bool ShouldClose(bool value = false, bool set = false) noexcept;           // 应用关闭

        // 发送指令处理
        void SendMessage(const RpcMessage& message, size_t retryCount = 0);

        // 心跳处理
        void OnHeartbeat();

        // 创建服务实例
        void CreateService(const std::string& serviceName);
        // 获取服务示例
        std::shared_ptr<Service> GetService() const noexcept;
        uint64_t GetServiceId() const noexcept;
        
        // 获取请求ID
        uint64_t RequestId();

        // 获取对端地址
        const LikesProgram::Net::Address& GetRemoteAddress() const noexcept;
        // 获取本段地址
        const LikesProgram::Net::Address& GetLocalAddress() const noexcept;
    private:
        // 心跳超时处理
        void OnHeartbeatTimeout();
        // 请求超时处理
        void OnRequestTimeout(uint64_t requestId);
        // 重置心跳计时
        void ResetHeartbeatTimer();
    private:
        Connection* m_connection = nullptr;             // 服务控制器
        std::atomic_bool m_closed{false};               // 关闭标志
        std::atomic_bool m_shouldClose{false};          // 应用关闭标志

        LikesProgram::Net::Address m_remoteAddr;        // 对端地址
        LikesProgram::Net::Address m_localAddr;         // 本端地址

        uint64_t m_serviceId;                           // 服务ID
        std::shared_ptr<Service> m_service{nullptr};    // 服务实例

        uint32_t m_communicationCounter;                // 通信计数器
        
        int64_t m_heartbeatInterval = 30;               // 心跳间隔（秒）
        DeadlineScheduler::TimerId m_heartbeatTimerId;  // 心跳定时器ID
        int64_t m_heartbeatTimeoutCont = 0;             // 心跳超时次数
        int64_t m_heartbeatTimeoutLimit;                // 心跳超时次数上限

        int64_t m_timeoutMilliseconds = 1000*10;         // 请求超时时间（毫秒）
        int64_t m_requestRetryCount = 3;                 // 请求重试次数

        // 代返回请求（发送了请求，但是客户端还没返回）
        struct PendingRequest {
            RpcMessage message;
            size_t retryCount = 0; // 重试次数
            LikesProgram::Time::Timer timer; // 计时器 (用于计算延迟)
            DeadlineScheduler::TimerId timerId; // 超时定时器ID
        };
        std::unordered_map<uint64_t, PendingRequest> m_pending;
    };
}
