#pragma once
#include <LikesProgram/net/Buffer.hpp>
#include <LikeHub/RpcMessage.hpp>
#include <LikeHub/ServiceInstance.hpp>
#include <LikeHub/EventListener.hpp>
#include <memory>
#include <string>
#include <shared_mutex>

namespace LikeHub {
    class Session;
    class Service : public std::enable_shared_from_this<Service> {
    public:
        Service(std::weak_ptr<Session> session, uint64_t serviceId, const std::string& name, LikesProgram::Net::Address serviceAddress);
        void Register();
        ~Service();

        // 更新负载
        void UpdateLoad(InstanceMetricsSnapshot metrics);

        std::shared_ptr<Session> GetSession() const noexcept { return m_session.lock(); }

        uint64_t Id() const noexcept { return m_instance.instanceId; }

        const std::string& Name() const noexcept { return m_instance.serviceName; }

        const LikesProgram::Net::Address& Address() const noexcept { return m_instance.serviceAddress; }

        void SetStatus(LifecycleStatus status) noexcept { m_instance.lifecycle.store(status, std::memory_order_relaxed); }
        LifecycleStatus GetStatus() const noexcept { return m_instance.lifecycle.load(std::memory_order_relaxed); }

        void SetHealthStatus(HealthStatus status) noexcept { m_instance.health.store(status, std::memory_order_relaxed); }
        HealthStatus GetHealthStatus() const noexcept { return m_instance.health.load(std::memory_order_relaxed); }

        // 获取负载
        double GetLoad() const noexcept {
            auto lifecycle = GetStatus();
            auto health = GetHealthStatus();
            if (lifecycle != LifecycleStatus::Active) return std::numeric_limits<double>::max();
            double load = m_load.load(std::memory_order_relaxed);
            switch (health) {
                case HealthStatus::Healthy: return load;
                case HealthStatus::Suspected: return std::min(1.0, load + 0.3);
                case HealthStatus::Unknown: return std::min(1.0, load + 0.5);
                case HealthStatus::Unhealthy:
                case HealthStatus::Lost:
                default: return std::numeric_limits<double>::max();
            }
        }

        // 注册事件
        ServiceLoadSnapshot GetLoadSnapshot() const noexcept;
        ServiceSnapshot GetSnapshot() const noexcept;

        uint64_t RegisterEvent(const std::string& eventName, ListenType listenType = ListenType::Event);
        
        // 注销事件
        void UnregisterEvent(const std::string& eventName, uint64_t listenerId);
        bool BelongToListener(uint64_t eventListenerId) const;
    private:
        bool HandleEvent(const EventContext& ctx);
    private:
        std::weak_ptr<Session> m_session;
        ServiceInstance m_instance;

        // 事件索引
        std::unordered_map<uint64_t, std::string> m_eventsIdToName;   // listenerId -> 事件名称
        mutable std::shared_mutex m_eventsMutex;

        std::atomic<double> m_load{0.0}; // 综合负载，范围 [0.0, 1.0]
        InstanceMetricsSnapshot m_metrics;
        double m_cpuLoad = 0.0;
        double m_memLoad = 0.0;
        double m_stressLoad = 0.0;
        double m_latencyLoad = 0.0;
        mutable std::shared_mutex m_loadMutex;
    };
}
