#include <Service.hpp>
#include <Session.hpp>
#include <atomic>
#include <mutex>
#include <ServiceRegistry.hpp>
#include <LikeHub/RpcMessage.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <cmath>

namespace LikeHub {
    Service::Service(std::weak_ptr<Session> session, uint64_t serviceId, const std::string& name, LikesProgram::Net::Address serviceAddress): m_session(session) {
        m_instance.instanceId  = serviceId;
        m_instance.serviceName = name;
        m_instance.serviceAddress = serviceAddress;
    }
    void Service::Register() {
        ServiceRegistry::Instance().RegisterService(shared_from_this());
    }

    // 一定要先移除事件，否则可能注册中心通知报空指针
    Service::~Service() {
        // 移除事件注册
        for (const auto& [listenerId, eventName] : m_eventsIdToName) {
            ServiceRegistry::Instance().UnregisterEvent(eventName, listenerId);
        }
        
        // 移除服务注册
        ServiceRegistry::Instance().UnregisterService(Id());
    }

    
    void Service::UpdateLoad(InstanceMetricsSnapshot metrics) {
        double oldLoad = m_load.load(std::memory_order_relaxed);
        double cpu = std::clamp(metrics.cpuPct / 100.0, 0.0, 1.0);
        double mem = std::clamp(metrics.memPct / 100.0, 0.0, 1.0);
        double stress = std::clamp(metrics.stressIndicators / 100.0, 0.0, 1.0);
        double lat = std::clamp(metrics.delayMs / 1000.0, 0.0, 1.0);
        // 加权计算
        double load = 0.4 * cpu + 0.2 * mem + 0.3 * lat + 0.1 * stress;

        if (!std::isfinite(load)) {
            LogWarn(
                u"Service load update ignored: serviceId={} serviceName={} oldLoad={} cpuPct={} memPct={} stressIndicators={} delayMs={} reason=LoadNotFinite",
                Id(),
                Name(),
                oldLoad,
                metrics.cpuPct,
                metrics.memPct,
                metrics.stressIndicators,
                metrics.delayMs
            );
            return;
        }
    
        m_load.store(load, std::memory_order_relaxed);
    
        LogDebug(
            u"Service load updated: serviceId={} serviceName={} lifecycle={} health={} oldLoad={} newLoad={} cpuPct={} memPct={} stressIndicators={} delayMs={} cpu={} mem={} qps={} lat={}",
            Id(),
            Name(),
            static_cast<int>(GetStatus()),
            static_cast<int>(GetHealthStatus()),
            oldLoad,
            load,
            metrics.cpuPct,
            metrics.memPct,
            metrics.stressIndicators,
            metrics.delayMs,
            cpu,
            mem,
            stress,
            lat
        );
    }

    uint64_t Service::RegisterEvent(const std::string& eventName, ListenType listenType) {
        auto session = m_session.lock();
        if (!session) {
            LogWarn(u"Service register event failed: session expired eventName={} serviceId={} serviceName={}", eventName, Id(), Name());
            return 0;
        }
        // 参数验证
        if (eventName.empty() || listenType == ListenType::Unknown) {
            LogWarn(
                u"Service register event failed: invalid argument eventName={} listenType={} serviceId={} serviceName={}",
                eventName, ListenTypeToString(listenType), Id(), Name()
            );
            return 0;
        }

        // 构建处理函数
        auto self = shared_from_this();
        std::weak_ptr<Service> weakSelf = self;
        EventHandler handler = [weakSelf, listenType](const EventContext& ctx) -> bool {
            auto targetService = weakSelf.lock();
            if (!targetService) {
                LogWarn(u"Service event handler rejected: eventName={} target service expired", ctx.eventName);
                return false;
            }
            RpcMessage message = ctx.message;
            if (ctx.source && listenType == ListenType::Event) {
                // 返回值 Meta
                LikesProgram::Configuration responseMeta;
                responseMeta[u"message"] = u"Event successfully delivered";
                ctx.source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::OK, responseMeta, 0));
            }
            return targetService->HandleEvent(ctx);
        };

        // 执行注册
        std::string actualListenerName = Name() + "." + eventName;
        auto& registry = ServiceRegistry::Instance();
        uint64_t listenerId = 0;
        listenerId = registry.RegisterEvent(self, eventName, actualListenerName, std::move(handler), listenType);
        if (listenerId == 0) {
            LogWarn(
                u"Service register event failed: eventName={} listenerName={} listenType={} serviceId={} serviceName={}",
                eventName, actualListenerName, ListenTypeToString(listenType), Id(), Name()
            );
            return 0;
        }

        // 记录事件索引
        {
            std::unique_lock lock(m_eventsMutex);
            m_eventsIdToName[listenerId] = eventName;
        }

        LogInfo(
            u"Service event registered: listenerId={} eventName={} listenerName={} listenType={} serviceId={} serviceName={}",
            listenerId, eventName, actualListenerName, ListenTypeToString(listenType), Id(), Name()
        );

        return listenerId;
    }

    void Service::UnregisterEvent(const std::string& eventName, uint64_t listenerId) {
        // 参数验证
        if (eventName.empty() || listenerId == 0) return;

        // 注册事件的名称
        std::string registeredEventName;
        {
            std::unique_lock lock(m_eventsMutex);
            auto it = m_eventsIdToName.find(listenerId);
            if (it == m_eventsIdToName.end()) return;
            registeredEventName = it->second;

            if (registeredEventName != eventName) {
                LogWarn(
                    u"Service unregister event rejected: listenerId={} expectedEventName={} inputEventName={} serviceId={} serviceName={}",
                    listenerId, registeredEventName, eventName, Id(), Name()
                );
                return;
            }
            m_eventsIdToName.erase(listenerId);
        }

        // 从注册中心中移除事件
        ServiceRegistry::Instance().UnregisterEvent(eventName, listenerId);
        LogDebug(
            u"Service event unregistered: listenerId={} eventName={} serviceId={} serviceName={}",
            listenerId, eventName, Id(), Name()
        );
    }

    bool Service::BelongToListener(uint64_t eventListenerId) const {
        if (eventListenerId == 0) return false;
        std::shared_lock lock(m_eventsMutex);
        return m_eventsIdToName.contains(eventListenerId);
    }

    bool Service::HandleEvent(const EventContext& ctx) {
        auto session = m_session.lock();
        if (!session) {
            LogWarn(
                u"Service handle event failed: session expired eventName={} targetServiceId={} targetServiceName={}",
                ctx.eventName, Id(), Name()
            );
            return false;
        }

        if (session->ShouldClose()) {
            LogWarn(
                u"Service handle event failed: session closing eventName={} targetServiceId={} targetServiceName={}",
                ctx.eventName, Id(), Name()
            );
            return false;
        }

        LogDebug(
            u"Service event forwarding: eventName={} targetServiceId={} targetServiceName={} sourceServiceId={} requestId={}",
            ctx.eventName, Id(), Name(), ctx.source ? ctx.source->GetServiceId() : 0, ctx.message.Header().requestId
        );

        // 转发
        try {
            session->SendMessage(ctx.message);
            return true;
        } catch (const std::exception& e) {
            LogWarn(
                u"Service event forwarding exception: error={} eventName={} targetServiceId={} targetServiceName={} requestId={}",
                e.what(), ctx.eventName, Id(), Name(), ctx.message.Header().requestId
            );
            return false;
        } catch (...) {
            LogWarn(
                u"Service event forwarding unknown exception: eventName={} targetServiceId={} targetServiceName={} requestId={}",
                ctx.eventName, Id(), Name(), ctx.message.Header().requestId
            );
            return false;
        }
    }
}