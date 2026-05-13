#include <DefaultEventHandlers.hpp>
#include <ServiceRegistry.hpp>
#include <Session.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <LikesProgram/log/Logger.hpp>

/* 注册默认处理事件
* 只由服务发送，无需注册处理的事件：
*   GetServiceInfo      : 获取服务信息
*   Heartbeat           : 心跳消息
* 特殊事件，注册后会自动通知
*   ServiceChanged     : 监听节点信息，用于监控面板
*/
namespace LikeHub {
    // 注册 登录事件处理
    static void RegisterLoginEventHandler(ServiceRegistry& registry) {
        registry.RegisterEvent(0, "Login", "DefaultEventHandler", [](const EventContext& ctx) -> bool {
            std::shared_ptr<Session> source = ctx.source;
            if (!source) {
                LogWarn(u"Default Login handler rejected: eventName={} source session is null", ctx.eventName);
                return true; // 登录失败不重发
            }

            // 返回值 Meta
            LikesProgram::Configuration responseMeta;
            
            LikesProgram::String remoteAddress = u"0.0.0.0:0";
            if (ctx.source) remoteAddress = LikesProgram::String(ctx.source->GetRemoteAddress().ToString());
            LogDebug(
                u"Default Login handler received: eventName={} source={}",
                ctx.eventName, remoteAddress
            );

            RpcMessage message = ctx.message;

            LikesProgram::String serviceName = u"Unknown"; // 服务名称
            LikesProgram::String userName = u""; // 用户名
            LikesProgram::String password = u""; // 密码
            try {
                const auto& metadata = message.MetaData();
                if (!metadata[u"serviceName"].IsNull()) serviceName = metadata[u"serviceName"].AsString();
                if (!metadata[u"userName"].IsNull()) userName = metadata[u"userName"].AsString();
                if (!metadata[u"password"].IsNull()) password = metadata[u"password"].AsString();
            } catch (const std::exception& e) {
                LogWarn(
                    u"Hub service Login metadata parse failed: error={} serviceIp={} serviceId={} requestId={}",
                    e.what(), source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId
                );
                // 发送错误响应
                responseMeta[u"message"] = "Invalid Login metadata";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta, 0));
                return true; // 登录失败不重发
            } catch (...) {
                LogWarn(
                    u"Hub service Login metadata parse failed: unknown exception serviceIp={} serviceId={} requestId={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId
                );
                // 发送错误响应
                responseMeta[u"message"] = "Invalid Login metadata";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta, 0));
                return true; // 登录失败不重发
            }
            if (serviceName.Empty()) serviceName = u"Unknown";
            if (userName.Empty() || password.Empty()) {
                LogWarn(
                    u"Hub service Login rejected: missing username or password serviceIp={} serviceId={} requestId={} serviceName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName
                );

                // 发送错误响应
                responseMeta[u"message"] = "Missing username or password";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta, 0));
                // 关闭连接
                source->ShouldClose(true, true);
                return true; // 登录失败不重发
            }

            bool authenticated = false;
            try {
                authenticated = ServiceRegistry::Instance().Authentication(userName, password);
            } catch (const std::exception& e) {
                LogWarn(
                    u"Hub service Login authentication exception: error={} serviceIp={} serviceId={} requestId={} serviceName={} userName={}",
                    e.what(), source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, userName
                );
                
                // 发送错误响应
                responseMeta[u"message"] = "Authentication service exception";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::InternalError, responseMeta, 0));
                // 关闭连接
                source->ShouldClose(true, true);
                return true; // 登录失败不重发
            } catch (...) {
                LogWarn(
                    u"Hub service Login authentication unknown exception: serviceIp={} serviceId={} requestId={} serviceName={} userName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, userName
                );

                // 发送错误响应
                responseMeta[u"message"] = "Authentication service unknown exception";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::InternalError, responseMeta, 0));
                // 关闭连接
                source->ShouldClose(true, true);
                return true; // 登录失败不重发
            }

            // 账号或密码错误
            if (!authenticated) {
                LogWarn(
                    u"Hub service Login failed: serviceIp={} serviceId={} requestId={} serviceName={} userName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, userName
                );

                // 发送错误响应
                responseMeta[u"message"] = "Invalid username or password";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::Unauthorized, responseMeta, 0));
                // 关闭连接
                source->ShouldClose(true, true);
                return true; // 登录失败不重发
            }
            // 创建服务实例
            source->CreateService(serviceName.ToStdString());
            LogInfo(
                u"Hub service Login success: serviceIp={} serviceId={} requestId={} serviceName={} userName={}",
                source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, userName
            );
            // 发送错误响应
            responseMeta[u"message"] = "Login success";
            source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::OK, responseMeta, 0));
            return true;
        }, ListenType::Event, {}, [](const EventFailureContext& ctx) -> EventDecision {
            (void) ctx;
            return EventDecision::Ignore;
        }, {}, 0);
        LogInfo(u"Default event handlers registered: Login");
    }

    // 注册 远程注册事件处理
    static void RegisterRemoteRegisterEventHandlers(ServiceRegistry& registry) {
        registry.RegisterEvent(0, "RegisterEvent", "DefaultEventHandler", [](const EventContext& ctx) -> bool {
            std::shared_ptr<Session> source = ctx.source;
            if (!source) {
                LogWarn(u"Default RegisterEvent handler rejected: eventName={} source session is null", ctx.eventName);
                return true; // 事件注册失败不重发
            }
            
            LikesProgram::String remoteAddress = u"0.0.0.0:0";
            if (ctx.source) remoteAddress = LikesProgram::String(ctx.source->GetRemoteAddress().ToString());
            LogDebug(
                u"Default RegisterEvent handler received: eventName={} source={}",
                ctx.eventName, remoteAddress
            );

            RpcMessage message = ctx.message;

            // 返回值 Meta
            LikesProgram::Configuration responseMeta;

            // 获取服务实例
            std::shared_ptr<Service> service = source->GetService();
            if (!service) { // 服务实例未创建，未登录
                LogWarn(u"Service not logged in serviceIp={} serviceId={} requestId={}", source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId);
                responseMeta[u"message"] = "Service not logged in";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::Forbidden, responseMeta, 0));
                return true;  // 事件注册失败不重发
            }

            std::string serviceName = service->Name();      // 服务名称
            LikesProgram::String eventName;                 // 事件名称
            ListenType listenType = ListenType::Unknown;    // 获取指令动词
            try {
                const auto& metadata = message.MetaData();
                if (!metadata[u"eventName"].IsNull()) eventName = metadata[u"eventName"].AsString();
                if (!metadata[u"listenType"].IsNull()) listenType = UInt8ToListenType(metadata[u"listenType"].AsInt());
            } catch (const std::exception& e) {
                LogWarn(
                    u"Hub service RegisterEvent metadata parse failed: error={} serviceIp={} serviceId={} requestId={}",
                    e.what(), source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId
                );
                // 发送错误响应
                responseMeta[u"message"] = "Invalid RegisterEvent metadata";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta, 0));
                return true; // 事件注册失败不重发
            } catch (...) {
                LogWarn(
                    u"Hub service RegisterEvent metadata parse failed: unknown exception serviceIp={} serviceId={} requestId={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId
                );
                // 发送错误响应
                responseMeta[u"message"] = "Invalid RegisterEvent metadata";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta, 0));
                return true; // 事件注册失败不重发
            }

            // 参数验证
            if (eventName.Empty()) {
                LogWarn(
                    u"Hub service RegisterEvent rejected: missing eventName serviceIp={} serviceId={} requestId={} serviceName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName
                );
                responseMeta[u"message"] = "Missing eventName";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta, 0));
                return true; // 事件注册失败不重发
            }
            if (listenType == ListenType::Unknown) {
                LogWarn(
                    u"Hub service RegisterEvent rejected: invalid listenType serviceIp={} serviceId={} requestId={} serviceName={} eventName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName
                );
                responseMeta[u"message"] = "Invalid listenType";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta, 0));
                return true; // 事件注册失败不重发
            }

            uint64_t listenerId = 0;
            try {
                listenerId = service->RegisterEvent(eventName.ToStdString(), listenType);
            } catch (const std::exception& e) {
                LogWarn(
                    u"Hub service RegisterEvent exception: error={} serviceIp={} serviceId={} requestId={} serviceName={} eventName={}",
                    e.what(), source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName
                );
                responseMeta[u"message"] = "Register event exception";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::InternalError, responseMeta, 0));
                return true; // 事件注册失败不重发
            } catch (...) {
                LogWarn(
                    u"Hub service RegisterEvent unknown exception: serviceIp={} serviceId={} requestId={} serviceName={} eventName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName
                );
                responseMeta[u"message"] = "Register event unknown exception";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::InternalError, responseMeta, 0));
                return true; // 事件注册失败不重发
            }

            if (listenerId <= 0) {
                LogWarn(
                    u"Hub service RegisterEvent failed: serviceIp={} serviceId={} requestId={} serviceName={} eventName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName
                );
                responseMeta[u"message"] = "Register event failed";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::Conflict, responseMeta, 0));
                return true; // 事件注册失败不重发
            }

            LogInfo(
                u"Hub service RegisterEvent success: serviceIp={} serviceId={} requestId={} serviceName={} eventName={} listenerId={} listenType={}",
                source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName, listenerId, LikesProgram::String(ListenTypeToString(listenType))
            );
            responseMeta[u"message"] = "Register event success";
            responseMeta[u"data"] = static_cast<int64_t>(listenerId);
            source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::OK, responseMeta, 0));
            return true;
        }, ListenType::Event, {}, [](const EventFailureContext& ctx) -> EventDecision {
            (void) ctx;
            return EventDecision::Ignore;
        }, {}, 0);
        LogInfo(u"Default event handlers registered: RegisterEvent");
    }

    // 注册 获取服务列表
    static void RegisterGetServiceListEventHandlers(ServiceRegistry& registry) {
        registry.RegisterEvent(0, "GetServiceList", "DefaultEventHandler", [](const EventContext& ctx) -> bool {
            std::shared_ptr<Session> source = ctx.source;
            if (!source) {
                LogWarn(u"Default GetServiceList handler rejected: eventName={} source session is null", ctx.eventName);
                return true; // 事件注册失败不重发
            }
            
            LikesProgram::String remoteAddress = u"0.0.0.0:0";
            if (ctx.source) remoteAddress = LikesProgram::String(ctx.source->GetRemoteAddress().ToString());
            LogDebug(
                u"Default GetServiceList handler received: eventName={} source={}",
                ctx.eventName, remoteAddress
            );

            RpcMessage message = ctx.message;

            // 返回值 Meta
            LikesProgram::Configuration responseMeta;

            // 获取服务实例
            std::shared_ptr<Service> service = source->GetService();
            if (!service) { // 服务实例未创建，未登录
                LogWarn(u"Service not logged in serviceIp={} serviceId={} requestId={}", source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId);
                responseMeta[u"message"] = "Service not logged in";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::Forbidden, responseMeta, 0));
                return true; // 事件注册失败不重发
            }

            std::string serviceName = service->Name();      // 服务名称
            LikesProgram::String eventName;                 // 事件名称
            ListenType listenType = ListenType::Unknown;    // 获取指令动词
            try {
                const auto& metadata = message.MetaData();
                if (!metadata[u"eventName"].IsNull()) eventName = metadata[u"eventName"].AsString();
                if (!metadata[u"listenType"].IsNull()) listenType = UInt8ToListenType(metadata[u"listenType"].AsInt());
            } catch (const std::exception& e) {
                LogWarn(
                    u"Hub service GetServiceList metadata parse failed: error={} serviceIp={} serviceId={} requestId={}",
                    e.what(), source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId
                );
                // 发送错误响应
                responseMeta[u"message"] = "Invalid GetServiceList metadata";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta, 0));
                return true; // 事件注册失败不重发
            } catch (...) {
                LogWarn(
                    u"Hub service GetServiceList metadata parse failed: unknown exception serviceIp={} serviceId={} requestId={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId
                );
                // 发送错误响应
                responseMeta[u"message"] = "Invalid GetServiceList metadata";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta, 0));
                return true; // 事件注册失败不重发
            }

            // 参数验证
            if (eventName.Empty()) {
                LogWarn(
                    u"Hub service GetServiceList rejected: missing eventName serviceIp={} serviceId={} requestId={} serviceName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName
                );
                responseMeta[u"message"] = "Missing eventName";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta, 0));
                return true; // 事件注册失败不重发
            }
            if (listenType == ListenType::Unknown) {
                LogWarn(
                    u"Hub service GetServiceList rejected: invalid listenType serviceIp={} serviceId={} requestId={} serviceName={} eventName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName
                );
                responseMeta[u"message"] = "Invalid listenType";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta, 0));
                return true; // 事件注册失败不重发
            }

            uint64_t listenerId = 0;
            try {
                listenerId = service->RegisterEvent(eventName.ToStdString(), listenType);
            } catch (const std::exception& e) {
                LogWarn(
                    u"Hub service GetServiceList exception: error={} serviceIp={} serviceId={} requestId={} serviceName={} eventName={}",
                    e.what(), source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName
                );
                responseMeta[u"message"] = "Register event exception";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::InternalError, responseMeta, 0));
                return true; // 事件注册失败不重发
            } catch (...) {
                LogWarn(
                    u"Hub service GetServiceList unknown exception: serviceIp={} serviceId={} requestId={} serviceName={} eventName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName
                );
                responseMeta[u"message"] = "Register event unknown exception";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::InternalError, responseMeta, 0));
                return true; // 事件注册失败不重发
            }

            if (listenerId <= 0) {
                LogWarn(
                    u"Hub service GetServiceList failed: serviceIp={} serviceId={} requestId={} serviceName={} eventName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName
                );
                responseMeta[u"message"] = "Register event failed";
                source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::Conflict, responseMeta, 0));
                return true; // 事件注册失败不重发
            }

            LogInfo(
                u"Hub service GetServiceList success: serviceIp={} serviceId={} requestId={} serviceName={} eventName={} listenerId={} listenType={}",
                source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName, listenerId, LikesProgram::String(ListenTypeToString(listenType))
            );
            responseMeta[u"message"] = "Register event success";
            responseMeta[u"data"] = static_cast<int64_t>(listenerId);
            source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, message.Header().requestId, RpcMessage::Code::OK, responseMeta, 0));
            return true;
        }, ListenType::Event, {}, [](const EventFailureContext& ctx) -> EventDecision {
            (void) ctx;
            return EventDecision::Ignore;
        }, {}, 0);
        LogInfo(u"Default event handlers registered: GetServiceList");
    }

    // 注册默认处理事件
    void RegisterDefaultEventHandlers() {
        auto& registry = ServiceRegistry::Instance();
        // Login
        RegisterLoginEventHandler(registry);                // 登录事件
        // RegisterEvent
        RegisterRemoteRegisterEventHandlers(registry);      // 远程事件注册
        // GetServiceList
        RegisterGetServiceListEventHandlers(registry);      // 获取服务列表
    }

}