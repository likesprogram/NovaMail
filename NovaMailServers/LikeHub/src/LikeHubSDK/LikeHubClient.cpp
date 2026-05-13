#include <LikeHub/LikeHubClient.hpp>
#include <LikeHub/SystemMetrics.hpp>
#include <LikesProgram/net/transports/TcpTransport.hpp>
#include <LikesProgram/net/Connection.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace {
    class ClientConnection final : public LikesProgram::Net::Connection {
    public:
        ClientConnection(SocketType fd, LikesProgram::Net::EventLoop* loop, std::unique_ptr<LikesProgram::Net::Transport> transport)
            : Connection(fd, loop, std::move(transport)) {
            LogDebug(u"Client new conn[{}] this[{:P}]", (long long)GetSocket(), (const void*)this);
        }

        ~ClientConnection() {
            LogDebug(u"Client del conn[{}] this[{:P}]", (long long)GetSocket(), (const void*)this);
        }

    protected:
        void OnConnected() override { }

        void OnMessage(LikesProgram::Net::Buffer& in) override;

        void OnClosed() override {
            // 连接关闭
            std::string serverAddress = "{" + GetRemoteAddress().ToString() + "}";
            std::string clientAddress = "{" + GetLocalAddress().ToString() + "}";
            LogWarn(u"NovaMailHub Client closed: Server address: {}, Client address: {}", serverAddress, clientAddress);
            try {
                LikeHub::LikeHubClient::Instance().NotifyConnectionClosed("LikeHub connection closed before login completed");
            } catch (...) { }
        }

        void OnError(int err) override {
            LogWarn(u"NovaMailHub Client connection error: err={}", err);
            try {
                LikeHub::LikeHubClient::Instance().NotifyConnectionError("LikeHub connection error: " + std::to_string(err));
            } catch (...) { }
        }
    };

    // 创建连接器工厂
    LikesProgram::Net::ConnectionFactory MakeConnection = [](SocketType fd, LikesProgram::Net::EventLoop* ownerLoop) -> std::shared_ptr<LikesProgram::Net::Connection> {
        auto transport = std::make_unique<LikesProgram::Net::TcpTransport>(fd);
        return std::make_shared<ClientConnection>(fd, ownerLoop, std::move(transport));
    };
}

namespace LikeHub {
    static LikesProgram::String ParseAction(const RpcMessage& request) {
        LikesProgram::String action = u"Unknown";
        try {
            const auto& metadata = request.MetaData();
            if (!metadata[u"action"].IsNull()) action = metadata[u"action"].AsString();
        } catch (const std::exception& e) {
            LogWarn(u"Hub service request action parse failed: error={} requestId={}", e.what(), request.Header().requestId);
        } catch (...) {
            LogWarn(u"Hub service request action parse failed: unknown error requestId={}", request.Header().requestId);
        }
        return action;
    }
    std::shared_ptr<LikeHubClient> LikeHubClient::s_instance = nullptr;
    std::once_flag LikeHubClient::s_initFlag;

    LikeHubClient& LikeHubClient::Instance() {
        if (!s_instance) {
            throw std::logic_error(
                "LikeHubClient has not been initialized. "
                "Call LikeHubClient::Initialize(...) first."
            );
        }
        return *s_instance;
    }

    void LikeHubClient::Initialize(
        const LikesProgram::Net::Address& remoteAddr, const LikesProgram::String& serviceName,
        const LikesProgram::String& access, const LikesProgram::String& secret,
        std::function<void()> loginSuccess,  std::function<void(std::string)> loginFailure
    ) {
        std::call_once(s_initFlag, [&]() {
            s_instance  = std::shared_ptr<LikeHubClient>(new LikeHubClient(remoteAddr, serviceName, access, secret, loginSuccess, loginFailure));
        });
    }

    LikesProgram::Net::Client& LikeHubClient::GetClient() {
        return m_client;
    }

    void LikeHubClient::OnMessage(const RpcMessage& message) {
        // 根据消息类型进行处理
        auto self = shared_from_this();
        switch (RpcMessage::UInt8ToType(message.Header().requestType))
        {
        case RpcMessage::Type::Request: {
            LikesProgram::String action = ParseAction(message);
            if (action == u"GetServiceInfo") {
                OnGetServiceInfo(message);
            } else if (action == u"Heartbeat") {
                OnHeartbeat(message.Header().requestId);
            } else {
                EventHandler handler;
                {
                    std::lock_guard<std::mutex> lock(m_eventsMutex);
                    auto it = m_events.find(action.ToStdString());
                    if (it == m_events.end()) {
                        // 未订阅的事件被错误派发
                        LogWarn(
                            u"Unsubscribed events were mistakenly dispatched: action={} responseType={} serviceId={} requestId={} code={}",
                            action, RpcMessage::TypeToString(RpcMessage::UInt8ToType(message.Header().requestType)), m_serviceId, message.Header().requestId, message.Header().code
                        );
                        return;
                    }
                    // 获取处理器
                    handler = it->second;
                }
                EventContext context;
                context.eventName = action.ToStdString();
                context.source = nullptr;
                context.message = message;
                try {
                    // 执行
                    bool success = handler(context);
                    if (!success) {
                        LogWarn(
                            u"Hub event listener returned false: eventName={} requestId={}",
                            action, message.Header().requestId
                        );
                    }
                } catch (const std::exception& e) {
                    LogWarn(
                        u"Hub event listener exception: eventName={} requestId={} error={}",
                        action, message.Header().requestId, e.what()
                    );
                } catch (...) {
                    LogWarn(
                        u"Hub event listener unknown exception: eventName={} requestId={}",
                        action, message.Header().requestId
                    );
                }
            }
        } break;
        case RpcMessage::Type::Response: {
            auto it = m_pending.find(message.Header().requestId);
            if (it == m_pending.end()) {
                LogWarn(u"Hub message response but not found in pending: requestId={}", message.Header().requestId);
                return;
            }
            PendingRequest pendingMessage = std::move(it->second);
            // 取消定时器
            if (pendingMessage.timerId != 0) DeadlineScheduler::Instance().Cancel(pendingMessage.timerId);
            // 移除 pending 记录
            m_pending.erase(it);

            // 获取返回代码
            int code = message.Header().code;

            // 失败返回
            if (code != static_cast<int>(RpcMessage::Code::OK)) {
                LogWarn(
                    u"Hub message request failed: code={} requestType={} serviceId={} requestId={} metaLength={} bodyLength={} delay={}",
                    code, RpcMessage::TypeToString(RpcMessage::UInt8ToType(message.Header().requestType)), m_serviceId,
                    message.Header().requestId, message.Header().metaLength, message.Header().bodyLength, LikesProgram::Time::NsToMs(pendingMessage.timer.Stop().count())
                );
                RpcMessage requestMessage = pendingMessage.message;
                LikesProgram::String action = ParseAction(requestMessage);
                if (action == u"Classification") { // 智能分类处理失败，自己发送邮件送达请求
                    LikesProgram::Configuration newEmailDeliveryMetadata = requestMessage.MetaData();
                    newEmailDeliveryMetadata[u"action"] = u"NewEmailDelivery";
                    SendMessage(LikeHub::RpcMessage::Encode(LikeHub::RpcMessage::Type::Request, 0, LikeHub::RpcMessage::Code::None, newEmailDeliveryMetadata, 0), 0);
                }
                return;
            }

            // 获取请求时的 请求指令
            LikesProgram::String requestAction = ParseAction(pendingMessage.message);

            // 空值判断
            if (requestAction == u"Unknown" || requestAction.Empty()) {
                LogWarn(
                    u"Hub message response missing action: responseType={} serviceId={} requestId={} code={} delay={}",
                    RpcMessage::TypeToString(RpcMessage::UInt8ToType(message.Header().requestType)),
                    m_serviceId, message.Header().requestId, code, LikesProgram::Time::NsToMs(pendingMessage.timer.Stop().count())
                );
                return;
            }

            // 登录返回
            if (requestAction == u"Login") LoginResponse(message);
            
            // 其他返回
            LogDebug(
                u"Hub message response succeeded: requestType={} serviceId={} requestId={} metaLength={} bodyLength={} delay={}",
                RpcMessage::TypeToString(RpcMessage::UInt8ToType(message.Header().requestType)), m_serviceId,
                message.Header().requestId, message.Header().metaLength, message.Header().bodyLength, LikesProgram::Time::NsToMs(pendingMessage.timer.Stop().count())
            );
        } break;
        default: { // 未知消息类型
            LogWarn(
                u"Unknown Hub message: requestType={} serviceId={} requestId={} code={}",
                RpcMessage::TypeToString(RpcMessage::UInt8ToType(message.Header().requestType)), m_serviceId, message.Header().requestId, message.Header().code
            );
        } break;
        }
    }

    void LikeHubClient::SendMessage(const RpcMessage& message, size_t retryCount) {
        uint64_t requestId = message.Header().requestId;
        if (requestId == 0) requestId = RequestId();
        uint32_t timeoutMs = message.Header().timeoutMs;
        if (timeoutMs == 0) timeoutMs = m_timeoutMilliseconds;

        // 发送指令
        auto conn = m_client.GetConnection();
        if (!conn) {
            LikesProgram::String action = ParseAction(message);
            LogWarn(u"Hub client SendMessage failed: connection is null action={} requestId={}", action, requestId);
            if (action == u"Login") NotifyFailureOnce("LikeHub login failed: connection is null");
        }
        LikesProgram::Net::Buffer buf = message.ToBuffer(requestId);
        conn->Send(buf);

        // 记录代返回请求
        if (message.Header().requestType == static_cast<uint8_t>(RpcMessage::Type::Request)) {
            std::weak_ptr<LikeHubClient> weakSelf = weak_from_this();
            // 构造 PendingRequest
            PendingRequest pending{
                .message = message,
                .retryCount = retryCount,
                .timer = LikesProgram::Time::Timer(true), // 启动计时器
                .timerId = DeadlineScheduler::Instance().ScheduleAfter(std::chrono::milliseconds(timeoutMs), [weakSelf, requestId]() {
                    auto self = weakSelf.lock();
                    if (!self) return;
                    auto conn = self->m_client.GetConnection();
                    if (!conn) return;
                    // 通过事件循环线程安全调用
                    conn->RunInLoop([weakSelf, requestId]() {
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
    void LikeHubClient::NotifyConnectionClosed(const std::string& reason) {
        (void) reason;
        // NotifyFailureOnce(reason);
    }

    void LikeHubClient::NotifyConnectionError(const std::string& reason) {
        NotifyFailureOnce(reason);
    }
    void LikeHubClient::RegisterEvent(const LikesProgram::String& eventName, EventHandler handler, ListenType type) {
        if (eventName.Empty() || !handler) return;
        if (type == ListenType::Unknown) {
            LogWarn(u"Attempt to register event with unknown listen type: eventName={}", eventName);
            return;
        }

        const std::string eventKey = eventName.ToStdString();
        // 先保存本地 handler，避免 Hub 注册成功后事件立刻派发，但本地还没记录
        {
            std::lock_guard<std::mutex> lock(m_eventsMutex);
            m_events[eventKey] = std::move(handler);
        }

        // 发送消息
        LikesProgram::Configuration metadata;
        metadata[u"action"] = u"RegisterEvent";
        metadata[u"eventName"] = eventName;
        metadata[u"listenType"] = static_cast<int64_t>(ListenTypeToUInt8(type));
        SendMessage(RpcMessage::Encode(RpcMessage::Type::Request, RequestId(), RpcMessage::Code::None, metadata, m_timeoutMilliseconds));
        LogInfo(u"Hub client local event registered: eventName={} listenType={}", eventName, ListenTypeToString(type));
    }

    void LikeHubClient::SendLoginRequest() {
        LikesProgram::Configuration metadata;
        metadata[u"action"] = u"Login";
        metadata[u"serviceName"] = m_serviceName;
        metadata[u"userName"] = m_access;
        metadata[u"password"] = m_secret;
        SendMessage(RpcMessage::Encode(RpcMessage::Type::Request, RequestId(), RpcMessage::Code::None, metadata, m_timeoutMilliseconds));
    }

    void LikeHubClient::LoginResponse(const RpcMessage& response) {
        if (response.Header().code == static_cast<int>(RpcMessage::Code::OK)) {
            LogInfo(u"NovaMailHub Login success serviceId={} requestId={}", m_serviceId, response.Header().requestId);
            m_loginSuccess.store(true, std::memory_order_release);
            try { if (m_loginSuccessCallback) m_loginSuccessCallback(); } catch(const std::exception& e) {
                LogWarn(u"NovaMailHub login success callback exception: {}", e.what());
            } catch (...) {
                LogWarn(u"NovaMailHub login success callback unknown exception");
            }
            return;
        }
        std::string reason = "LikeHub login rejected, code=" + std::to_string(response.Header().code) + ", requestId=" + std::to_string(response.Header().requestId);

        LogWarn(u"NovaMailHub Login rejected serviceId={} requestId={} code={}", m_serviceId, response.Header().requestId, response.Header().code);
        NotifyFailureOnce(reason);
    }

    void LikeHubClient::NotifyFailureOnce(const std::string& message) {
        if (m_loginSuccess.load(std::memory_order_acquire)) return;
        bool expected = false;
        if (!m_failureNotified.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
        LogWarn(u"NovaMailHub client failed: {}", message);
        try {
            if (m_loginFailureCallback) m_loginFailureCallback(message);
        } catch (const std::exception& e) {
            LogWarn(u"NovaMailHub failure callback exception: {}", e.what());
        } catch (...) {
            LogWarn(u"NovaMailHub failure callback unknown exception");
        }
    }

    void LikeHubClient::OnGetServiceInfo(const RpcMessage& message) {
        auto conn = m_client.GetConnection();
        if (!conn) {
            LogWarn(u"OnGetServiceInfo failed: connection is null");
            return;
        }
        const uint64_t requestId = message.Header().requestId;
        std::weak_ptr<LikeHubClient> weakSelf = weak_from_this();
        conn->RunInLoop([weakSelf, requestId]() {
            auto self = weakSelf.lock();
            if (!self) return;
            LikesProgram::Configuration metadata;
            metadata[u"action"] = u"GetServiceInfo";
            metadata[u"serviceId"] = static_cast<int64_t>(self->m_serviceId);
            metadata[u"serviceName"] = self->m_serviceName;
            metadata[u"message"] = u"GetServiceInfo success";
        
            self->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, requestId, RpcMessage::Code::OK, metadata, self->m_timeoutMilliseconds));
            // 发送登录信息
            self->SendLoginRequest();
        });
    }

    void LikeHubClient::OnHeartbeat(uint64_t requestId) {
        auto snapshot = SystemMetrics::Instance().Sample();
        LikesProgram::Configuration metrics;
        metrics[u"cpuPct"] = snapshot.cpuPct;
        metrics[u"memPct"] = snapshot.memPct;
        metrics[u"stressIndicators"] = snapshot.stressIndicators;

        LikesProgram::Configuration metadata;
        metadata[u"action"] = u"Heartbeat";
        metadata[u"metrics"] = metrics;
        metadata[u"message"] = u"Heartbeat OK";
        SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, requestId, RpcMessage::Code::OK, metadata, m_timeoutMilliseconds));
    }

    void LikeHubClient::OnRequestTimeout(uint64_t requestId) {
        auto it = m_pending.find(requestId);
        if (it == m_pending.end()) {
            LogWarn(u"Hub service request timeout but not found in pending: requestId={}", requestId);
            return;
        }

        PendingRequest pendingRequest = std::move(it->second);
        m_pending.erase(it); // 移除 pending 记录

        RpcMessage message = pendingRequest.message;
        LikesProgram::String action = ParseAction(message);

        LogWarn(u"Hub service request timeout handled: action={} requestType={} requestId={}", action, RpcMessage::TypeToString(RpcMessage::UInt8ToType(message.Header().requestType)), message.Header().requestId);
        if (action == u"Login") NotifyFailureOnce("LikeHub login timeout, requestId=" + std::to_string(requestId));
    }

    LikeHubClient::~LikeHubClient() {
        m_client.Shutdown(); // 关闭客户端
        m_client.WaitShutdown(); // 等待客户端关闭完成
    }

    LikeHubClient::LikeHubClient(
        const LikesProgram::Net::Address& remoteAddr, const LikesProgram::String& serviceName,
        const LikesProgram::String& access, const LikesProgram::String& secret,
        std::function<void()> loginSuccess,  std::function<void(std::string)> loginFailure
    ) : m_client(remoteAddr, MakeConnection), m_serviceName(serviceName), m_access(access), m_secret(secret), m_loginSuccessCallback(std::move(loginSuccess)), m_loginFailureCallback(std::move(loginFailure)) {
        // 启动客户端
        m_client.Start();
    }

    uint64_t LikeHubClient::RequestId() {
        for (int i = 0; i < 1024; ++i) { // 限制尝试次数
            uint32_t local = m_communicationCounter++;
            if (local == 0) continue; // 跳过 0

            uint64_t id = (static_cast<uint64_t>(m_serviceId) << 32) | local;

            if (!m_pending.contains(id)) return id;
        }

        // 极端情况
        throw std::runtime_error("requestId exhausted");
    }
}

namespace {
    void ClientConnection::OnMessage(LikesProgram::Net::Buffer& in) {
        LikeHub::LikeHubClient& client = LikeHub::LikeHubClient::Instance();
        for (;;) {
            // 解析指令
            LikeHub::RpcMessage::ParseResult result = LikeHub::RpcMessage::TryParse(in, 1024 * 1024 * 1024);
            // 判断解析结果
            if (result.status == LikeHub::RpcMessage::ParseStatus::Success) {
                if (!result.message) {
                    LogError(u"Hub client parse success but message is null from {}", GetRemoteAddress().ToString());
                    client.NotifyConnectionError("Hub client parse success but message is null");
                    Shutdown();
                    break;
                }
                client.OnMessage(*result.message); // 解析成功，处理消息
                continue; // 开启下一次循环
            }
            else if (result.status == LikeHub::RpcMessage::ParseStatus::Incomplete) return; // 等待更多数据
            else if (result.status == LikeHub::RpcMessage::ParseStatus::InvalidMagic) {
                LogError(u"Invalid magic from {}: {}", GetRemoteAddress().ToString(), result.errorMsg);
                client.NotifyConnectionError("Invalid magic from LikeHub server: " + result.errorMsg);
                Shutdown();
                break;
            } else {
                LogError(u"Failed to parse message from {}: {}", GetRemoteAddress().ToString(), result.errorMsg);
                LikesProgram::Configuration metadata; // 构造错误响应元数据
                metadata["message"] = LikesProgram::String(result.errorMsg);
                client.SendMessage(LikeHub::RpcMessage::Encode(LikeHub::RpcMessage::Type::Response, 0, LikeHub::RpcMessage::Code::BadRequest, metadata, 0)); // 发送错误响应
                break;
            }
        }
    }
}
