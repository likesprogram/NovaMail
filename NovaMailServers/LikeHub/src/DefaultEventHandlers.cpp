#include <DefaultEventHandlers.hpp>
#include <ServiceRegistry.hpp>
#include <Session.hpp>
#include <Service.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace LikeHub {
    static void SendResponse(
        const std::shared_ptr<Session>& source,
        uint64_t requestId,
        RpcMessage::Code code,
        const LikesProgram::Configuration& responseMeta
    ) {
        if (!source) return;
        source->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, requestId, code, responseMeta, 0));
    }

    static void RegisterLoginEventHandler(ServiceRegistry& registry) {
        registry.RegisterEvent(nullptr, "Login", "DefaultEventHandler", [](const EventContext& ctx) -> bool {
            std::shared_ptr<Session> source = ctx.source;
            if (!source) {
                LogWarn(u"Default Login handler rejected: eventName={} source session is null", ctx.eventName);
                return true;
            }

            RpcMessage message = ctx.message;
            LikesProgram::Configuration responseMeta;
            LikesProgram::String serviceName = u"Unknown";
            LikesProgram::String userName;
            LikesProgram::String password;

            LikesProgram::String remoteAddress = u"0.0.0.0:0";
            if (ctx.source) remoteAddress = LikesProgram::String(ctx.source->GetRemoteAddress().ToString());
            LogDebug(u"Default Login handler received: eventName={} source={}", ctx.eventName, remoteAddress);

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
                responseMeta[u"message"] = "Invalid Login metadata";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta);
                return true;
            } catch (...) {
                LogWarn(
                    u"Hub service Login metadata parse failed: unknown exception serviceIp={} serviceId={} requestId={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId
                );
                responseMeta[u"message"] = "Invalid Login metadata";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta);
                return true;
            }

            if (serviceName.Empty()) serviceName = u"Unknown";
            if (userName.Empty() || password.Empty()) {
                LogWarn(
                    u"Hub service Login rejected: missing username or password serviceIp={} serviceId={} requestId={} serviceName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName
                );
                responseMeta[u"message"] = "Missing username or password";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta);
                source->ShouldClose(true, true);
                return true;
            }

            bool authenticated = false;
            try {
                authenticated = ServiceRegistry::Instance().Authentication(userName, password);
            } catch (const std::exception& e) {
                LogWarn(
                    u"Hub service Login authentication exception: error={} serviceIp={} serviceId={} requestId={} serviceName={} userName={}",
                    e.what(), source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, userName
                );
                responseMeta[u"message"] = "Authentication service exception";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::InternalError, responseMeta);
                source->ShouldClose(true, true);
                return true;
            } catch (...) {
                LogWarn(
                    u"Hub service Login authentication unknown exception: serviceIp={} serviceId={} requestId={} serviceName={} userName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, userName
                );
                responseMeta[u"message"] = "Authentication service unknown exception";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::InternalError, responseMeta);
                source->ShouldClose(true, true);
                return true;
            }

            if (!authenticated) {
                LogWarn(
                    u"Hub service Login failed: serviceIp={} serviceId={} requestId={} serviceName={} userName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, userName
                );
                responseMeta[u"message"] = "Invalid username or password";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::Unauthorized, responseMeta);
                source->ShouldClose(true, true);
                return true;
            }

            source->CreateService(serviceName.ToStdString());
            LogInfo(
                u"Hub service Login success: serviceIp={} serviceId={} requestId={} serviceName={} userName={}",
                source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, userName
            );

            responseMeta[u"message"] = "Login success";
            SendResponse(source, message.Header().requestId, RpcMessage::Code::OK, responseMeta);
            return true;
        }, ListenType::Event, {}, [](const EventFailureContext& ctx) -> EventDecision {
            (void) ctx;
            return EventDecision::Ignore;
        }, {}, 0);
        LogInfo(u"Default event handlers registered: Login");
    }

    static void RegisterRemoteRegisterEventHandlers(ServiceRegistry& registry) {
        registry.RegisterEvent(nullptr, "RegisterEvent", "DefaultEventHandler", [](const EventContext& ctx) -> bool {
            std::shared_ptr<Session> source = ctx.source;
            if (!source) {
                LogWarn(u"Default RegisterEvent handler rejected: eventName={} source session is null", ctx.eventName);
                return true;
            }

            RpcMessage message = ctx.message;
            LikesProgram::Configuration responseMeta;

            LikesProgram::String remoteAddress = LikesProgram::String(source->GetRemoteAddress().ToString());
            LogDebug(u"Default RegisterEvent handler received: eventName={} source={}", ctx.eventName, remoteAddress);

            std::shared_ptr<Service> service = source->GetService();
            if (!service) {
                LogWarn(u"Service not logged in serviceIp={} serviceId={} requestId={}", source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId);
                responseMeta[u"message"] = "Service not logged in";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::Forbidden, responseMeta);
                return true;
            }

            std::string serviceName = service->Name();
            LikesProgram::String eventName;
            ListenType listenType = ListenType::Unknown;

            try {
                const auto& metadata = message.MetaData();
                if (!metadata[u"eventName"].IsNull()) eventName = metadata[u"eventName"].AsString();
                if (!metadata[u"listenType"].IsNull()) listenType = UInt8ToListenType(metadata[u"listenType"].AsInt());
            } catch (const std::exception& e) {
                LogWarn(
                    u"Hub service RegisterEvent metadata parse failed: error={} serviceIp={} serviceId={} requestId={}",
                    e.what(), source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId
                );
                responseMeta[u"message"] = "Invalid RegisterEvent metadata";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta);
                return true;
            } catch (...) {
                LogWarn(
                    u"Hub service RegisterEvent metadata parse failed: unknown exception serviceIp={} serviceId={} requestId={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId
                );
                responseMeta[u"message"] = "Invalid RegisterEvent metadata";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta);
                return true;
            }

            if (eventName.Empty()) {
                LogWarn(
                    u"Hub service RegisterEvent rejected: missing eventName serviceIp={} serviceId={} requestId={} serviceName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName
                );
                responseMeta[u"message"] = "Missing eventName";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta);
                return true;
            }
            if (listenType == ListenType::Unknown) {
                LogWarn(
                    u"Hub service RegisterEvent rejected: invalid listenType serviceIp={} serviceId={} requestId={} serviceName={} eventName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName
                );
                responseMeta[u"message"] = "Invalid listenType";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::BadRequest, responseMeta);
                return true;
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
                SendResponse(source, message.Header().requestId, RpcMessage::Code::InternalError, responseMeta);
                return true;
            } catch (...) {
                LogWarn(
                    u"Hub service RegisterEvent unknown exception: serviceIp={} serviceId={} requestId={} serviceName={} eventName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName
                );
                responseMeta[u"message"] = "Register event unknown exception";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::InternalError, responseMeta);
                return true;
            }

            if (listenerId == 0) {
                LogWarn(
                    u"Hub service RegisterEvent failed: serviceIp={} serviceId={} requestId={} serviceName={} eventName={}",
                    source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName
                );
                responseMeta[u"message"] = "Register event failed";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::Conflict, responseMeta);
                return true;
            }

            LogInfo(
                u"Hub service RegisterEvent success: serviceIp={} serviceId={} requestId={} serviceName={} eventName={} listenerId={} listenType={}",
                source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId, serviceName, eventName, listenerId, ListenTypeToString(listenType)
            );
            responseMeta[u"message"] = "Register event success";
            responseMeta[u"data"] = static_cast<int64_t>(listenerId);
            SendResponse(source, message.Header().requestId, RpcMessage::Code::OK, responseMeta);
            return true;
        }, ListenType::Event, {}, [](const EventFailureContext& ctx) -> EventDecision {
            (void) ctx;
            return EventDecision::Ignore;
        }, {}, 0);
        LogInfo(u"Default event handlers registered: RegisterEvent");
    }

    static void RegisterGetServiceListEventHandlers(ServiceRegistry& registry) {
        registry.RegisterEvent(nullptr, "GetServiceList", "DefaultEventHandler", [](const EventContext& ctx) -> bool {
            std::shared_ptr<Session> source = ctx.source;
            if (!source) {
                LogWarn(u"Default GetServiceList handler rejected: eventName={} source session is null", ctx.eventName);
                return true;
            }

            RpcMessage message = ctx.message;
            LikesProgram::Configuration responseMeta;

            std::shared_ptr<Service> requester = source->GetService();
            if (!requester) {
                LogWarn(u"Service not logged in serviceIp={} serviceId={} requestId={}", source->GetRemoteAddress().ToString(), source->GetServiceId(), message.Header().requestId);
                responseMeta[u"message"] = "Service not logged in";
                SendResponse(source, message.Header().requestId, RpcMessage::Code::Forbidden, responseMeta);
                return true;
            }

            LikesProgram::Configuration servicesMeta;
            const auto serviceSnapshots = ServiceRegistry::Instance().GetServiceSnapshots();
            for (const auto& serviceSnapshot : serviceSnapshots) {
                servicesMeta.Push_back(ServiceRegistry::BuildServiceMetadata(serviceSnapshot));
            }

            responseMeta[u"message"] = "GetServiceList success";
            responseMeta[u"count"] = static_cast<int64_t>(serviceSnapshots.size());
            responseMeta[u"data"] = servicesMeta;
            responseMeta[u"services"] = servicesMeta;

            LogInfo(
                u"Hub service GetServiceList success: serviceIp={} serviceId={} requestId={} serviceName={} count={}",
                source->GetRemoteAddress().ToString(),
                source->GetServiceId(),
                message.Header().requestId,
                requester->Name(),
                serviceSnapshots.size()
            );
            SendResponse(source, message.Header().requestId, RpcMessage::Code::OK, responseMeta);
            return true;
        }, ListenType::Event, {}, [](const EventFailureContext& ctx) -> EventDecision {
            (void) ctx;
            return EventDecision::Ignore;
        }, {}, 0);
        LogInfo(u"Default event handlers registered: GetServiceList");
    }

    void RegisterDefaultEventHandlers() {
        auto& registry = ServiceRegistry::Instance();
        RegisterLoginEventHandler(registry);
        RegisterRemoteRegisterEventHandlers(registry);
        RegisterGetServiceListEventHandlers(registry);
    }
}
