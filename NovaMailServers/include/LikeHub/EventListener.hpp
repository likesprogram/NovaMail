#pragma once
#include <LikeHub/RpcMessage.hpp>
#include <string>
#include <functional>
#include <memory>
#include <cstdint>

namespace LikeHub {
    class Session;
    class Service;
    // 监听类型
    enum class ListenType : uint8_t {
        Event = 0,   // 事件监听：根据负载均衡策略选择一个监听者
        Service = 1, // 服务监听：广播给所有监听者
        Unknown  // 未知事件：错误类型
    };
    inline ListenType UInt8ToListenType(uint8_t v) noexcept {
        if (v < static_cast<uint8_t>(ListenType::Unknown)) return static_cast<ListenType>(v);
        return ListenType::Unknown;
    }
    inline uint8_t ListenTypeToUInt8(ListenType type) noexcept {
        return static_cast<uint8_t>(type);
    }
    inline std::string ListenTypeToString(ListenType type) noexcept {
        switch (type) {
            case ListenType::Event: return "Event";
            case ListenType::Service: return "Service";
            default: return "Unknown";
        }
    }

    // 事件执行决策
    enum class EventDecision : uint8_t {
        Ignore,       // 忽略，不再处理
        Retry,        // 重试
        CloseSource,  // 关闭来源会话
        CloseTarget,  // 关闭目标服务会话
        COUNT         // 数量标记
    };
    inline EventDecision UInt8ToEventDecision(uint8_t v) noexcept {
        if (v < static_cast<uint8_t>(EventDecision::COUNT)) return static_cast<EventDecision>(v);
        return EventDecision::Ignore;
    }
    inline uint8_t EventDecisionToUInt8(EventDecision type) noexcept {
        return static_cast<uint8_t>(type);
    }
    inline std::string EventDecisionToString(EventDecision decision) {
        switch (decision) {
            case EventDecision::Ignore:
                return "Ignore";
            case EventDecision::Retry:
                return "Retry";
            case EventDecision::CloseSource:
                return "CloseSource";
            case EventDecision::CloseTarget:
                return "CloseTarget";
            default:
                return "Unknown";
        }
    }

    // 事件策略
    struct EventPolicy {
        bool retryOnFailure = false;        // handler 返回 false 或抛异常时是否允许重试
        bool retryOnTimeout = false;        // 远程事件超时时是否允许重试
        EventDecision failureDecision = EventDecision::Ignore; // 失败默认动作
        EventDecision timeoutDecision = EventDecision::Ignore; // 超时默认动作
    };


    // 事件上下文
    struct EventContext {
        std::string eventName;              // 事件名称
        std::shared_ptr<Session> source;    // 事件来源，Hub 主动触发时可以为空
        RpcMessage message;                 // 事件消息
    };

    // 失败上下文
    struct EventFailureContext {
        std::string eventName;              // 事件名称
        uint64_t listenerId = 0;            // 监听器 ID
        std::string listenerName;           // 监听器名称
        uint64_t ownerServiceId = 0;        // 所属服务 ID
        std::weak_ptr<Service> ownerService;// 所属服务

        std::shared_ptr<Session> source;    // 事件来源
        RpcMessage message;                 // 原始事件消息

        size_t retryCount = 0;              // 当前已经重试次数
        std::string reason;                 // 失败原因
    };

    // 超时上下文
    struct EventTimeoutContext {
        std::string eventName;              // 事件名称
        uint64_t listenerId = 0;            // 监听器 ID
        std::string listenerName;           // 监听器名称
        uint64_t ownerServiceId = 0;        // 所属服务 ID
        std::weak_ptr<Service> ownerService;// 所属服务

        std::shared_ptr<Session> source;    // 事件来源
        RpcMessage message;                 // 原始事件消息

        size_t retryCount = 0;              // 当前已经重试次数
    };

    // 处理函数
    using EventHandler = std::function<bool(const EventContext&)>;

    // 失败处理函数
    using EventFailureHandler = std::function<EventDecision(const EventFailureContext&)>;

    // 超时处理函数
    using EventTimeoutHandler = std::function<EventDecision(const EventTimeoutContext&)>;

    // 事件监听
    struct EventListener {
        uint64_t listenerId = 0;              // 监听 ID
        std::string listenerName;             // 监听名称
        uint64_t ownerServiceId = 0;          // 所属 Service ID，0 表示 Hub 内部处理器
        std::weak_ptr<Service> ownerService;  // 服务实例指针

        EventHandler handler;                 // 事件处理函数
        EventFailureHandler failureHandler;   // 事件失败处理函数
        EventTimeoutHandler timeoutHandler;   // 事件超时处理函数

        EventPolicy policy;                   // 事件处理策略

        ListenType type = ListenType::Event;  // 监听类型
        int priority = 0;                     // 优先级，可用于外部服务覆盖默认处理器
    };
}