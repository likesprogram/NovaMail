#include <EventDispatcher.hpp>
#include <Session.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <Service.hpp>
#include <cmath>

namespace LikeHub {
    static EventDecision HandleFailure(const EventListener& listener, const std::string& eventName, const RpcMessage& message, const std::shared_ptr<Session>& source, const std::string& reason) {
        if (listener.failureHandler) {
            EventFailureContext failureContext;
            failureContext.eventName = eventName;
            failureContext.listenerId = listener.listenerId;
            failureContext.listenerName = listener.listenerName;
            failureContext.ownerServiceId = listener.ownerServiceId;
            failureContext.source = source;
            failureContext.message = message;
            failureContext.retryCount = 0;
            failureContext.reason = reason;
            return listener.failureHandler(failureContext);
        }
        return listener.policy.failureDecision;
    }

    static void ApplyDecision(EventDecision decision, const EventListener& listener, const std::shared_ptr<Session>& source) {
        switch (decision) {
            case EventDecision::Ignore: break; // 忽略
            case EventDecision::Retry: break; // 重试
            case EventDecision::CloseSource: // 关闭消息来源
                if (source) source->ShouldClose(true, true);
                break;
            case EventDecision::CloseTarget: { // 关闭服务端
                auto service = listener.ownerService.lock();
                if (service) service->GetSession()->ShouldClose();
            } break;
            default: break; // 忽略
        }
    }

    static bool InvokeListener(const EventListener& listener, const std::string& eventName, const RpcMessage& message, const std::shared_ptr<Session>& source) {
        EventContext context;
        context.eventName = eventName;
        context.source = source;
        context.message = message;
        try {
            bool success = listener.handler(context);
            if (!success) {
                LogWarn(
                    u"Hub event listener returned false: eventName={} listenerId={} listenerName={} ownerServiceId={} requestId={}",
                    eventName, listener.listenerId, listener.listenerName, listener.ownerServiceId, message.Header().requestId
                );
                EventDecision decision = HandleFailure(listener, eventName, message, source, "handler returned false");
                ApplyDecision(decision, listener, source);
                return false;
            }
            return true;
        } catch (const std::exception& e) {
            LogWarn(
                u"Hub event listener exception: eventName={} listenerId={} listenerName={} ownerServiceId={} requestId={} error={}",
                eventName, listener.listenerId, listener.listenerName, listener.ownerServiceId, message.Header().requestId, e.what()
            );
            EventDecision decision = HandleFailure(listener, eventName, message, source, e.what());
            ApplyDecision(decision, listener, source);
            return false;
        } catch (...) {
            LogWarn(
                u"Hub event listener unknown exception: eventName={} listenerId={} listenerName={} ownerServiceId={} requestId={}",
                eventName, listener.listenerId, listener.listenerName, listener.ownerServiceId, message.Header().requestId
            );
            EventDecision decision = HandleFailure(listener, eventName, message, source, "unknown exception");
            ApplyDecision(decision, listener, source);
            return false;
        }
    }

    EventDispatcher& EventDispatcher::Instance() {
        static EventDispatcher instance;
        return instance;
    }

    EventTriggerResult EventDispatcher::Trigger(const std::string& eventName, const RpcMessage& message, const std::shared_ptr<Session>& source) {
        EventTriggerResult result;
        result.eventName = eventName;

        auto validListeners = GetValidListeners(eventName, source, result.listenType);
        if (validListeners.empty()) return result;

        result.matchedCount = validListeners.size();
        auto targets = SelectTargets(eventName, validListeners, result.listenType);
        result.targetCount = targets.size();

        for (const auto& listener : targets) {
            bool success = InvokeListener(listener, eventName, message, source);
            if (success) ++result.successCount;
            else ++result.failedCount;
        }

        return result;
    }

    std::vector<EventListener> EventDispatcher::GetValidListeners(const std::string& eventName, const std::shared_ptr<Session>& source, ListenType& listenType) {
        auto snapshot = ServiceRegistry::Instance().GetEventListeners(eventName);
        listenType = snapshot.type;
        if (snapshot.Empty()) return {};

        std::vector<EventListener> validListeners;
        validListeners.reserve(snapshot.listeners.size());

        std::shared_ptr<Service> sourceService = nullptr;
        if (source) sourceService = source->GetService();

        for (const auto& listener : snapshot.listeners) {
            if (!listener.handler) continue; // 排除没有处理函数

            // 排除自己
            if (sourceService && sourceService->BelongToListener(listener.listenerId)) continue;

            validListeners.push_back(listener);
        }

        std::stable_sort(validListeners.begin(), validListeners.end(), [](const EventListener& lhs, const EventListener& rhs) {
            return lhs.priority > rhs.priority;
        });

        return validListeners;
    }

    std::vector<EventListener> EventDispatcher::SelectTargets(const std::string& eventName, const std::vector<EventListener>& listeners, ListenType listenType) {
        std::vector<EventListener> targets;
        if (listeners.empty()) return targets;

        switch (listenType) {
            case ListenType::Event: {
                // Event：只选择一个
                auto selected = SelectOneByLoadBalance(eventName, listeners);
                if (selected.has_value()) targets.push_back(selected.value());
                break;
            }
            case ListenType::Service: {
                // Service：广播给所有有效监听器
                targets = listeners;
                break;
            }
            default: break;
        }

        return targets;
    }

    std::optional<EventListener> EventDispatcher::SelectOneByLoadBalance(const std::string& eventName, const std::vector<EventListener>& listeners) {
        if (listeners.empty()) return std::nullopt;

        struct Candidate {
            EventListener listener;
            double load = std::numeric_limits<double>::max();
        };

        std::vector<Candidate> candidates;
        candidates.reserve(listeners.size());

        // listeners 理论上已经按 priority 降序排好。
        // 这里仍然主动找最高优先级，避免调用方没排序导致问题。
        int highestPriority = std::numeric_limits<int>::min();

        for (const auto& listener : listeners) {
            if (!listener.handler) continue;

            if (listener.priority > highestPriority) highestPriority = listener.priority;
        }

        if (highestPriority == std::numeric_limits<int>::min()) return std::nullopt;

        // 只收集最高优先级的监听器
        for (const auto& listener : listeners) {
            if (!listener.handler) continue;

            if (listener.priority != highestPriority) continue;

            double load = ServiceRegistry::Instance().GetListenerLoad(listener);
            LogDebug(
                u"Event dispatch load checked: eventName={} listenerId={} listenerName={} ownerServiceId={} priority={} load={}",
                eventName,
                listener.listenerId,
                listener.listenerName,
                listener.ownerServiceId,
                listener.priority,
                load
            );

            // max 表示不可用
            if (!std::isfinite(load) || load >= std::numeric_limits<double>::max()) {
                LogWarn(
                    u"Event dispatch skip listener by invalid load: eventName={} listenerId={} listenerName={} ownerServiceId={} priority={} load={} reason=InvalidOrUnavailableLoad",
                    eventName,
                    listener.listenerId,
                    listener.listenerName,
                    listener.ownerServiceId,
                    listener.priority,
                    load
                );
                continue;
            }

            if (load < 0.0) load = 0.0;

            Candidate candidate;
            candidate.listener = listener;
            candidate.load = load;

            candidates.push_back(std::move(candidate));
        }

        if (candidates.empty()) {
            LogWarn(
                u"Event dispatch no available candidates: eventName={} listenerCount={} highestPriority={} reason=AllListenersUnavailableByLoad",
                eventName,
                listeners.size(),
                highestPriority
            );
            return std::nullopt;
        }

        // 按负载升序
        std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& lhs, const Candidate& rhs) {
            return lhs.load < rhs.load;
        });

        const double minLoad = candidates.front().load;

        std::vector<EventListener> minLoadListeners;
        minLoadListeners.reserve(candidates.size());

        for (const auto& candidate : candidates) {
            if (candidate.load != minLoad) break;
            minLoadListeners.push_back(candidate.listener);
        }

        if (minLoadListeners.empty()) return std::nullopt;

        if (minLoadListeners.size() == 1) return minLoadListeners.front();

        // 同优先级、同最低负载时，轮询选择一个
        size_t index = 0;

        {
            std::lock_guard<std::mutex> lock(m_loadBalanceMutex);
            size_t& cursor = m_roundRobinIndex[eventName];
            index = cursor % minLoadListeners.size();
            ++cursor;
        }

        return minLoadListeners[index];
    }
}