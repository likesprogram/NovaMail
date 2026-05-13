#include <Session.hpp>
#include <Connection.hpp>
#include <LikeHub/Snowflake.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <HandleDispatcher.hpp>
#include <ServiceRegistry.hpp>
#include <Service.hpp>
#include <LikesProgram/time/Timer.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace LikeHub {
    Session::Session(Connection* connection, LikesProgram::Net::Address remoteAddr, LikesProgram::Net::Address localAddr)
    : m_connection(connection), m_remoteAddr(remoteAddr), m_localAddr(localAddr), m_serviceId(0), m_communicationCounter(0),
    m_heartbeatTimerId(0) {
        // 获取配置
        LikeHub::LikeHubConfig& config = LikeHub::LikeHubConfig::Instance();
        LikesProgram::Configuration lkConfig = config.Raw();
        
        // 白名单判断
        bool release = false;
        LikesProgram::String remoteAddrStr(m_remoteAddr.Ip());
        for (const auto& whitelist : lkConfig[u"authentication"][u"whitelist"]) {
            if (
                !whitelist.AsString().Empty() && 
            (
                whitelist.AsString().EqualsIgnoreCase(remoteAddrStr) || 
                whitelist.AsString().EqualsIgnoreCase(u"*")
            )) {
                release = true;
                break;
            }
        }
        if (release) {
            LogDebug(
                u"Hub service connection accepted by whitelist: remoteIp={} remoteAddress={} localAddress={}",
                remoteAddrStr, m_remoteAddr.ToString(), m_localAddr.ToString()
            );
        } else {
            LogWarn(
                u"Hub service connection rejected by whitelist: remoteIp={} remoteAddress={} localAddress={}",
                remoteAddrStr, m_remoteAddr.ToString(), m_localAddr.ToString()
            );
            m_closed.store(true, std::memory_order_release); // 标记关闭
            ShouldClose(true, true);                         // 关闭业务
        }
        // 读取心跳相关配置
        m_heartbeatInterval = lkConfig[u"communication"][u"heartbeatInterval"].AsInt64();
        m_heartbeatTimeoutLimit = lkConfig[u"communication"][u"heartbeatTimeoutLimit"].AsInt64();
        // 读取请求相关配置
        m_timeoutMilliseconds = lkConfig[u"communication"][u"requestTimeoutMilliseconds"].AsInt64();
        m_requestRetryCount = lkConfig[u"communication"][u"requestRetryCount"].AsInt64();
    }

    Session::~Session() {
        bool expected = false;
        if (!m_closed.compare_exchange_strong(expected, true)) return;

        // 取消注册
        if (m_service) {
            m_service->SetStatus(LifecycleStatus::Deregistering);
            m_service.reset();
        }

        // 取消心跳定时器
        if (m_heartbeatTimerId != 0) {
            DeadlineScheduler::Instance().Cancel(m_heartbeatTimerId);
            m_heartbeatTimerId = 0;
        }

        // 取消所有 pending 请求的定时器
        for (auto& [requestId, pending] : m_pending) {
            if (pending.timerId != 0) DeadlineScheduler::Instance().Cancel(pending.timerId);
            pending.timerId = 0;
        }
    }

    void Session::OnConnect() {
        if (ShouldClose() || m_closed.load(std::memory_order_acquire)) return; // 连接关闭
        m_serviceId = Snowflake::NextId();

        LikesProgram::Configuration metadata;
        metadata[u"action"] = u"GetServiceInfo";
        metadata[u"serviceId"] = LikesProgram::String(std::to_string(m_serviceId));
        
        uint64_t requestId = RequestId();
        LogDebug(
            u"Hub service send getInfo: requestType={} serviceIp={} serviceId={} requestId={}",
            RpcMessage::TypeToString(RpcMessage::Type::Request), m_remoteAddr.ToString(),
            m_serviceId, requestId
        );

        SendMessage(RpcMessage::Encode(RpcMessage::Type::Request, requestId, RpcMessage::Code::None, metadata, m_timeoutMilliseconds));
    }

    void Session::OnMessage(const RpcMessage& message) {
        if (ShouldClose() || m_closed.load(std::memory_order_acquire)) return; // 连接关闭

        LikesProgram::String action = u"Unknown";
        if (!message.MetaData()[u"action"].IsNull()) action = message.MetaData()[u"action"].AsString();
        LogDebug(
            u"Hub service message received: requestType={} action={} serviceIp={} serviceId={} requestId={} metaLength={} bodyLength={}",
            RpcMessage::TypeToString(RpcMessage::UInt8ToType(message.Header().requestType)), action, m_remoteAddr.ToString(),
            m_serviceId, message.Header().requestId, message.Header().metaLength, message.Header().bodyLength
        );

        // 重置心跳计时器
        if (m_service) ResetHeartbeatTimer();

        // 根据消息类型进行处理
        auto self = shared_from_this();
        switch (RpcMessage::UInt8ToType(message.Header().requestType))
        {
        case RpcMessage::Type::Request:
            HandleDispatcher::HandleRequest(self, message);
            break;
        case RpcMessage::Type::Response: {
            auto it = m_pending.find(message.Header().requestId);
            if (it == m_pending.end()) {
                LogWarn(u"Hub service response but not found in pending: serviceIp={} requestId={}", m_remoteAddr.ToString(), message.Header().requestId);
                return;
            }
            PendingRequest pendingMessage = std::move(it->second);
            // 取消定时器
            if (pendingMessage.timerId != 0) DeadlineScheduler::Instance().Cancel(pendingMessage.timerId);
            // 移除 pending 记录
            m_pending.erase(it);
            HandleDispatcher::HandleResponse(self, message, pendingMessage.message, LikesProgram::Time::NsToMs(pendingMessage.timer.Stop().count()));
        } break;
        default:
            HandleDispatcher::HandleUnknown(self, message);
            break;
        }
    }

    void Session::ResetHeartbeatTimer() {
        if (ShouldClose() || m_closed.load(std::memory_order_acquire)) return;
        if (!m_service) return;

        if (m_heartbeatTimerId != 0) {
            DeadlineScheduler::Instance().Cancel(m_heartbeatTimerId);
            m_heartbeatTimerId = 0;
        }

        std::weak_ptr<Session> weakSelf = weak_from_this();

        m_heartbeatTimerId = DeadlineScheduler::Instance().ScheduleAfter(
            std::chrono::seconds(m_heartbeatInterval),
            [weakSelf]() {
                auto self = weakSelf.lock();
                if (!self) return;

                if (self->m_closed.load(std::memory_order_acquire)) return;

                Connection* connection = self->m_connection;
                if (!connection) return;
                connection->RunInLoop([weakSelf]() {
                    auto self = weakSelf.lock();
                    if (!self) return;
                    if (self->m_closed.load(std::memory_order_acquire)) return;
                    self->OnHeartbeat();
                });
            }
        );
    }

    bool Session::ShouldClose(bool value, bool set) noexcept {
        if (set) m_shouldClose.store(value, std::memory_order_release);
        return m_shouldClose.load(std::memory_order_acquire);
    }

    void Session::SendMessage(const RpcMessage& message, size_t retryCount) {
        if (ShouldClose() || m_closed.load(std::memory_order_acquire)) return;
        if (m_serviceId == 0) m_serviceId = Snowflake::NextId();
        uint64_t requestId = message.Header().requestId;
        if (requestId == 0) requestId = RequestId();
        uint32_t timeoutMs = message.Header().timeoutMs;
        if (timeoutMs == 0) timeoutMs = m_timeoutMilliseconds;

        // 发送指令
        LikesProgram::Net::Buffer buf = message.ToBuffer(requestId);
        m_connection->Send(buf);

        // 记录代返回请求
        if (message.Header().requestType == static_cast<uint8_t>(RpcMessage::Type::Request)) {
            std::weak_ptr<Session> weakSelf = weak_from_this();
            // 构造 PendingRequest
            PendingRequest pending{
                .message = message,
                .retryCount = retryCount,
                .timer = LikesProgram::Time::Timer(true), // 启动计时器
                .timerId = DeadlineScheduler::Instance().ScheduleAfter(std::chrono::milliseconds(timeoutMs), [weakSelf, requestId]() {
                    auto self = weakSelf.lock();
                    if (!self) return;
                    // 通过事件循环线程安全调用
                    self->m_connection->RunInLoop([weakSelf, requestId]() {
                        auto self = weakSelf.lock();
                        if (!self) return;
                        self->OnRequestTimeout(requestId);
                    });
                })
            };

            // 加入 pending
            m_pending.emplace(requestId, pending);
        }
    }

    void Session::OnHeartbeat() {
        if (ShouldClose() || m_closed.load(std::memory_order_acquire)) return;
        LogDebug(u"Heartbeat received from service: serviceIp={}", m_remoteAddr.ToString());
        LikesProgram::Configuration metadata;
        metadata[u"action"] = u"Heartbeat";
        SendMessage(RpcMessage::Encode(RpcMessage::Type::Request, RequestId(), RpcMessage::Code::None, metadata, m_timeoutMilliseconds));
    }

    void Session::CreateService(const std::string& serviceName) {
        // 空值判断
        if (ShouldClose() || m_closed.load(std::memory_order_acquire)) return;
        if (serviceName.empty()) {
            LogWarn(u"Create service failed: serviceName is empty remoteAddr={}", m_remoteAddr.ToString());
            return;
        }
        auto self = weak_from_this().lock();
        if (!self) {
            LogWarn(u"Create service failed: Session is not managed by shared_ptr remoteAddr={} serviceName={}", m_remoteAddr.ToString(), serviceName);
            return;
        }
        
        // 幂等性判断
        if (m_service) {
            LogWarn(
                u"Create service skipped: service already exists serviceId={} serviceName={} remoteAddr={}",
                m_serviceId, serviceName, m_remoteAddr.ToString()
            );
            return;
        }

        // 创建服务实例
        LogDebug(u"Create service: serviceId={} name={} remoteAddr={}", m_serviceId, serviceName, m_remoteAddr.ToString());
        m_service = std::make_shared<Service>(weak_from_this(), m_serviceId, serviceName, m_remoteAddr);

        // 注册到调度中心
        try { m_service->Register(); } catch (const std::exception& e) {
            LogError(
                u"Create service failed during init: error={} serviceId={} serviceName={} remoteAddr={}",
                e.what(), m_serviceId, serviceName, m_remoteAddr.ToString()
            );
            m_service.reset();
            throw;
        } catch (...) {
            LogError(
                u"Create service failed during init: unknown error serviceId={} serviceName={} remoteAddr={}",
                m_serviceId, serviceName, m_remoteAddr.ToString()
            );
            m_service.reset();
            throw;
        }
        LogDebug(
            u"Create service success: serviceId={} name={} remoteAddr={}",
            m_serviceId, serviceName, m_remoteAddr.ToString()
        );
        // 启动心跳计时器
        ResetHeartbeatTimer();
        // 先获取一次心跳包
        OnHeartbeat();
    }
    std::shared_ptr<Service> Session::GetService() const noexcept {
        return m_service;
    }
    
    uint64_t Session::GetServiceId() const noexcept {
        return m_serviceId;
    }

    uint64_t Session::RequestId() {
        for (int i = 0; i < 1024; ++i) { // 限制尝试次数
            uint32_t local = m_communicationCounter++;
            if (local == 0) continue; // 跳过 0

            uint64_t id = (static_cast<uint64_t>(m_serviceId) << 32) | local;

            if (!m_pending.contains(id)) return id;
        }

        // 极端情况
        throw std::runtime_error("requestId exhausted");
    }

    const LikesProgram::Net::Address& Session::GetRemoteAddress() const noexcept {
        return m_remoteAddr;
    }

    const LikesProgram::Net::Address& Session::GetLocalAddress() const noexcept {
        return m_localAddr;
    }

    void Session::OnHeartbeatTimeout() {
        if (ShouldClose() || m_closed.load(std::memory_order_acquire)) return;
        LogWarn(u"Heartbeat timeout for service: serviceIp={}", m_remoteAddr.ToString());
        ShouldClose(true, true); // 设置关闭标志，等待当前处理完成后优雅关闭
    }

    void Session::OnRequestTimeout(uint64_t requestId) {
        if (ShouldClose() || m_closed.load(std::memory_order_acquire)) return;
        
        auto it = m_pending.find(requestId);
        if (it == m_pending.end()) {
            LogWarn(u"Hub service request timeout but not found in pending: serviceIp={} requestId={}", m_remoteAddr.ToString(), requestId);
            return;
        }

        PendingRequest pendingRequest = std::move(it->second);
        m_pending.erase(it); // 移除 pending 记录

        // 心跳包超时
        try {
            if (!pendingRequest.message.MetaData()[u"action"].IsNull()) {
                if(pendingRequest.message.MetaData()[u"action"].AsString().EqualsIgnoreCase(u"heartbeat")) {
                    OnHeartbeatTimeout();
                    return;
                }
            }
        } catch(const std::exception& e) {
            LogError(u"OnRequestTimeout getAction exception: {}", e.what());
        } catch(...) {
            LogError(u"OnRequestTimeout getAction unknown exception");
        }
        

        // 执行超时处理
        HandleDispatcher::HandleTimeout(shared_from_this(), pendingRequest.message);
    }
}
