#pragma once
#include <LikeHub/EventListener.hpp>
#include <LikeHub/RpcMessage.hpp>
#include <ServiceRegistry.hpp>
#include <optional>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

namespace LikeHub {
    struct EventTriggerResult {
        std::string eventName;
        ListenType listenType = ListenType::Unknown;

        size_t matchedCount = 0;     // 过滤后的有效监听器数量
        size_t targetCount = 0;      // 实际派发目标数量

        size_t successCount = 0;     // 成功数量
        size_t failedCount = 0;      // 失败数量
        size_t ignoredCount = 0;     // 忽略数量
        size_t retryCount = 0;       // 总重试次数

        EventDecision finalDecision = EventDecision::Ignore;

        bool HasListener() const noexcept { return matchedCount > 0; }
        bool HasTarget() const noexcept { return targetCount > 0; }
        bool IsSuccess() const noexcept { return targetCount > 0 && failedCount == 0; }
    };

   class EventDispatcher {
    public:
        static EventDispatcher& Instance();
        EventTriggerResult Trigger(const std::string& eventName, const RpcMessage& message, const std::shared_ptr<Session>& source = nullptr);
    private:
        EventDispatcher() = default;
        std::vector<EventListener> GetValidListeners(const std::string& eventName, const std::shared_ptr<Session>& source, ListenType& listenType);
        std::vector<EventListener> SelectTargets(const std::string& eventName, const std::vector<EventListener>& listeners, ListenType listenType);
        std::optional<EventListener> SelectOneByLoadBalance(const std::string& eventName, const std::vector<EventListener>& listeners);
    private:
        std::mutex m_loadBalanceMutex;
        std::unordered_map<std::string, size_t> m_roundRobinIndex;
    };
}