#include <ServiceRegistry.hpp>
#include <Service.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <mutex>
#include <limits>
#include <cmath>
#include <shared_mutex>
#include <algorithm>
#include <EventDispatcher.hpp>
#include <DefaultEventHandlers.hpp>

namespace LikeHub {
    ServiceRegistry::ServiceRegistry() {
        // 获取配置
        LikeHub::LikeHubConfig& config = LikeHub::LikeHubConfig::Instance();
        LikesProgram::Configuration lkConfig = config.Raw();
        m_access = lkConfig[u"authentication"][u"access"].AsString();
        m_secret = lkConfig[u"authentication"][u"secret"].AsString();
        // 读取请求相关配置
        m_timeoutMilliseconds = lkConfig[u"communication"][u"requestTimeoutMilliseconds"].AsInt64();
        m_requestRetryCount = lkConfig[u"communication"][u"requestRetryCount"].AsInt64();
    }
    ServiceRegistry& ServiceRegistry::Instance() {
        static ServiceRegistry instance;
        return instance;
    }
    bool ServiceRegistry::Authentication(LikesProgram::String access, LikesProgram::String secret) {
        if (!m_access.Empty() && m_access != access) return false;
        if (!m_secret.Empty() && m_secret != secret) return false;
        return true;
    }

    void ServiceRegistry::RegisterService(const std::shared_ptr<Service>& service) {
        {
            std::unique_lock lock(m_servicesMutex);

            // 清理已经失效对象
            const uint64_t serviceId = service->Id();
            auto& vec = m_serviceNameMap[service->Name()];
            vec.erase(std::remove_if(vec.begin(), vec.end(), [serviceId](const WeakService& weakSvc) {
                auto locked = weakSvc.lock();
                if (!locked) return true;
                return locked->Id() == serviceId;
            }), vec.end());

            // 根据服务名称注册到服务列表
            vec.push_back(service);
            // 根据服务ID注册到服务ID映射
            m_serviceIdMap[service->Id()] = service;
        }

        // 通知服务监听者
        NotifyServiceChanged();

        LogInfo(u"Service registered: serviceId={} serviceName={} serviceAddress={}", service->Id(), service->Name(), service->Address().ToString());
    }

    void ServiceRegistry::UnregisterService(uint64_t serviceId) {
        if (serviceId == 0) return;

        std::string serviceName;
        std::string serviceAddress;
        {
            std::unique_lock lock(m_servicesMutex);
            auto idIt = m_serviceIdMap.find(serviceId);
            if (idIt != m_serviceIdMap.end()) {
                if (auto svc = idIt->second.lock()) {
                    serviceName = svc->Name();
                    serviceAddress = svc->Address().ToString();
                }
                m_serviceIdMap.erase(idIt);
            }

            // 从服务列表中移除
            if (!serviceName.empty()) {
                auto nameIt = m_serviceNameMap.find(serviceName);
                if (nameIt != m_serviceNameMap.end()) {
                    auto& vec = nameIt->second;
                    vec.erase(std::remove_if(vec.begin(), vec.end(), [serviceId](const WeakService& weakSvc) {
                        auto locked = weakSvc.lock();
                        if (!locked) return true;
                        return locked->Id() == serviceId;
                    }), vec.end());
                    if (vec.empty()) m_serviceNameMap.erase(nameIt);
                }
            } else {
                for (auto it = m_serviceNameMap.begin(); it != m_serviceNameMap.end();) {
                    auto& vec = it->second;
                    vec.erase(std::remove_if(vec.begin(), vec.end(), [serviceId](const WeakService& weakSvc) {
                        auto locked = weakSvc.lock();
                        if (!locked) return true;
                        return locked->Id() == serviceId;
                    }), vec.end());
                    if (vec.empty()) it = m_serviceNameMap.erase(it);
                    else ++it;
                }
            }
        }
        
        // 通知服务监听者
        NotifyServiceChanged();
        LogInfo(u"Service unregistered: serviceId={} serviceName={} serviceAddress={}", serviceId, serviceName.empty() ? "Unknown" : serviceName, serviceAddress.empty() ? "Unknown" : serviceAddress);
    }

    std::vector<std::shared_ptr<Service>> ServiceRegistry::GetServices(const std::string& serviceName) const {
        std::vector<std::shared_ptr<Service>> result;
        if (serviceName.empty()) return result;

        std::shared_lock lock(m_servicesMutex);
        auto it = m_serviceNameMap.find(serviceName);
        if (it == m_serviceNameMap.end()) return result;

        const auto& vec = it->second;
        result.reserve(vec.size());

        for (const auto& weakSvc : vec) {
            auto svc = weakSvc.lock();
            if (!svc) continue;
            result.push_back(std::move(svc));
        }
        
        return result;
    }

    std::shared_ptr<Service> ServiceRegistry::GetService(uint64_t serviceId) const {
        if (serviceId == 0) return nullptr;
        std::shared_lock lock(m_servicesMutex);
        auto it = m_serviceIdMap.find(serviceId);
        if (it == m_serviceIdMap.end()) return nullptr;
        return it->second.lock();
    }

    uint64_t ServiceRegistry::RegisterEvent(
        const std::shared_ptr<Service>& service, const std::string& eventName, const std::string& listenerName, EventHandler handler, ListenType type,
        EventPolicy policy, EventFailureHandler failureHandler, EventTimeoutHandler timeoutHandler, int priority
    ) {
        if (eventName.empty() || listenerName.empty() || !handler) return 0;
        if (type == ListenType::Unknown) {
            LogWarn(
                u"Attempt to register event with unknown listen type: eventName={} listenerName={}",
                eventName, listenerName
            );
            return 0;
        }

        // 创建事件监听
        EventListener listener;
        const uint64_t listenerId = m_nextListenerId.fetch_add(1, std::memory_order_relaxed);
        listener.listenerId = listenerId;
        listener.listenerName = listenerName;
        const uint64_t serviceId = service ? service->Id() : 0;
        listener.ownerServiceId = serviceId;
        listener.ownerService = service;
        listener.handler = std::move(handler);
        listener.failureHandler = std::move(failureHandler);
        listener.timeoutHandler = std::move(timeoutHandler);
        listener.policy = policy;
        listener.type = type;
        listener.priority = priority;
        {
            std::unique_lock lock(m_eventsMutex);
            auto& bucket = m_events[eventName];
            // 类型一致性检查
            if (!bucket.instances.empty() && bucket.type != type) {
                LogWarn(
                    u"Attempt to register event with conflicting listen type: eventName={} existingType={} newType={}",
                    eventName, bucket.type == ListenType::Event ? u"Event" : u"Service", type == ListenType::Event ? u"Event" : u"Service"
                );
                return 0; // 已有监听但类型不一致，拒绝注册
            }

            if (bucket.instances.empty()) bucket.type = type; // 首次注册设置类型

            auto& instances = bucket.instances;
            // 去重
            if (std::find_if(instances.begin(), instances.end(), [serviceId, &listenerName](const EventListener& listener) {
                return listener.ownerServiceId == serviceId && listener.listenerName == listenerName;
            }) != instances.end()) {
                LogWarn(u"Attempt to register duplicate event listener: serviceId={} listenerName={} eventName={}", serviceId, listenerName, eventName);
                return 0; // 已有该服务实例监听，拒绝重复注册
            }

            bucket.instances.push_back(std::move(listener));
        }
        LogInfo(u"Event registered: listenerId={} serviceId={} listenerName={} eventName={} listenType={}", listenerId, serviceId, listenerName, eventName, listener.type == ListenType::Event ? u"Event" : u"Service");
        return listenerId;
    }

    double ServiceRegistry::GetListenerLoad(const EventListener& listener) const noexcept {
        constexpr double kUnavailableLoad = std::numeric_limits<double>::max();
        constexpr double kInternalLoad = 0.0;
    
        try {
            if (!listener.handler) {
                LogWarn(
                    u"GetListenerLoad unavailable: listenerId={} listenerName={} ownerServiceId={} reason=NoHandler",
                    listener.listenerId,
                    listener.listenerName,
                    listener.ownerServiceId
                );
                return kUnavailableLoad;
            }
        
            // Hub 内部监听器
            if (listener.ownerServiceId == 0) {
                LogDebug(
                    u"GetListenerLoad internal listener: listenerId={} listenerName={} load={}",
                    listener.listenerId,
                    listener.listenerName,
                    kInternalLoad
                );
                return kInternalLoad;
            }
        
            auto service = listener.ownerService.lock();
            if (!service) {
                LogWarn(
                    u"GetListenerLoad unavailable: listenerId={} listenerName={} ownerServiceId={} reason=OwnerServiceExpired",
                    listener.listenerId,
                    listener.listenerName,
                    listener.ownerServiceId
                );
                return kUnavailableLoad;
            }
        
            double load = service->GetLoad();
        
            LogDebug(
                u"GetListenerLoad raw load: listenerId={} listenerName={} ownerServiceId={} load={}",
                listener.listenerId,
                listener.listenerName,
                listener.ownerServiceId,
                load
            );
        
            if (!std::isfinite(load)) {
                LogWarn(
                    u"GetListenerLoad unavailable: listenerId={} listenerName={} ownerServiceId={} load={} reason=LoadNotFinite",
                    listener.listenerId,
                    listener.listenerName,
                    listener.ownerServiceId,
                    load
                );
                return kUnavailableLoad;
            }
        
            if (load >= kUnavailableLoad) {
                LogWarn(
                    u"GetListenerLoad unavailable: listenerId={} listenerName={} ownerServiceId={} load={} reason=LoadIsUnavailableMax",
                    listener.listenerId,
                    listener.listenerName,
                    listener.ownerServiceId,
                    load
                );
                return kUnavailableLoad;
            }
        
            if (load < 0.0) {
                LogWarn(
                    u"GetListenerLoad abnormal negative load: listenerId={} listenerName={} ownerServiceId={} load={} fallback=0",
                    listener.listenerId,
                    listener.listenerName,
                    listener.ownerServiceId,
                    load
                );
                return 0.0;
            }
        
            return load;
        } catch (const std::exception& e) {
            LogWarn(
                u"GetListenerLoad exception: listenerId={} listenerName={} ownerServiceId={} error={}",
                listener.listenerId,
                listener.listenerName,
                listener.ownerServiceId,
                e.what()
            );
            return kUnavailableLoad;
        } catch (...) {
            LogWarn(
                u"GetListenerLoad unknown exception: listenerId={} listenerName={} ownerServiceId={}",
                listener.listenerId,
                listener.listenerName,
                listener.ownerServiceId
            );
            return kUnavailableLoad;
        }
    }

    void ServiceRegistry::NotifyServiceChanged() {
        LikesProgram::Configuration metadata;
        metadata[u"action"] = u"ServiceChanged";
        EventDispatcher::Instance().Trigger("ServiceChanged", RpcMessage::Encode(
            RpcMessage::Type::Request, 0, RpcMessage::Code::None, metadata, 0
        ), nullptr);
    }

    ServiceRegistry::EventBucketSnapshot ServiceRegistry::GetEventListeners(const std::string& eventName) const {
        std::shared_lock lock(m_eventsMutex);

        auto it = m_events.find(eventName);
        if (it == m_events.end()) return {};

        const auto& bucket = it->second;
        EventBucketSnapshot snapshot;
        snapshot.type = bucket.type;
        snapshot.listeners = bucket.instances;

        return snapshot;
    }

    void ServiceRegistry::UnregisterEvent(const std::string& eventName, uint64_t listenerId) {
        std::unique_lock lock(m_eventsMutex);

        auto it = m_events.find(eventName);
        if (it == m_events.end()) return; // 事件不存在

        // 移除事件
        auto& instances = it->second.instances;
        instances.erase(std::remove_if(instances.begin(), instances.end(), [listenerId](const EventListener& listener) {
            return listener.listenerId == listenerId;
        }), instances.end());

        LogInfo(u"Event unregistered: listenerId={} eventName={}", listenerId, eventName);

        // 移除空桶
        if (instances.empty()) it = m_events.erase(it);
    }
}