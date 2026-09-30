#pragma once
#include <LikeHub/EventListener.hpp>
#include <LikeHub/ServiceInstance.hpp>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>

namespace LikeHub {
    class Service;
    class ServiceRegistry {
    public:
        struct EventBucketSnapshot {
            ListenType type = ListenType::Unknown;
            std::vector<EventListener> listeners;

            bool Empty() const noexcept {
                return listeners.empty();
            }
        };
        using WeakService = std::weak_ptr<Service>;
        // 获取单例
        static ServiceRegistry& Instance();
        // 禁止拷贝
        ServiceRegistry(const ServiceRegistry&) = delete;
        ServiceRegistry& operator=(const ServiceRegistry&) = delete;
        // 禁止移动
        ServiceRegistry(ServiceRegistry&&) = delete;
        ServiceRegistry& operator=(ServiceRegistry&&) = delete;
        ~ServiceRegistry() = default;

        // 认证
        bool Authentication(LikesProgram::String access, LikesProgram::String secret);

        // 注册服务实例
        void RegisterService(const std::shared_ptr<Service>& service);
        // 注销服务实例
        void UnregisterService(uint64_t serviceId, const ServiceSnapshot& removedService = {});
        // 获取服务实例列表
        std::vector<std::shared_ptr<Service>> GetServices(const std::string& serviceName) const;
        std::vector<ServiceSnapshot> GetServiceSnapshots() const;
        // 获取单个实例
        std::shared_ptr<Service> GetService(uint64_t serviceId) const;
        
        // 注册监听事件
        uint64_t RegisterEvent(
            const std::shared_ptr<Service>& service, const std::string& eventName, const std::string& listenerName, EventHandler handler, ListenType type = ListenType::Event,
            EventPolicy policy = {}, EventFailureHandler failureHandler = {}, EventTimeoutHandler timeoutHandler = {}, int priority = 0
        );
        // 获取事件监听者
        EventBucketSnapshot GetEventListeners(const std::string& eventName) const;
        // 通知服务监听
        void NotifyServiceChanged(const ServiceSnapshot& changedService);
        static LikesProgram::Configuration BuildServiceMetadata(const ServiceSnapshot& service);
        // 获取延迟
        double GetListenerLoad(const EventListener& listener) const noexcept;
        // 注销事件
        void UnregisterEvent(const std::string& eventName, uint64_t listenerId);
    private:
        ServiceRegistry();
        struct EventBucket {
            ListenType type = ListenType::Event;        // 监听类型
            std::vector<EventListener> instances;       // 监听实例列表
            mutable std::atomic<uint32_t> rrIndex{0};   // 轮询索引（仅 ListenType::Event 有效）
        };
    private:
        LikesProgram::String m_access;                    // 账号
        LikesProgram::String m_secret;                    // 密码
        
        int64_t m_timeoutMilliseconds = 1000*10;         // 请求超时时间（毫秒）
        int64_t m_requestRetryCount = 3;                 // 请求重试次数

        std::unordered_map<std::string, EventBucket> m_events; // 事件名称 -> 监听桶
        mutable std::shared_mutex m_eventsMutex;

        std::unordered_map<std::string, std::vector<WeakService>> m_serviceNameMap;  // 服务名称 -> 服务实例列表 (仅 ListenType::Service 使用)
        std::unordered_map<uint64_t, WeakService> m_serviceIdMap; // 服务ID -> 服务实例映射（快速查找）

        mutable std::shared_mutex m_servicesMutex;
        std::atomic<uint64_t> m_nextListenerId{1};
    };
}
