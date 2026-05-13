#include <HandleDispatcher.hpp>
#include <EventDispatcher.hpp>
#include <ServiceRegistry.hpp>
#include <LikeHub/EventListener.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <Session.hpp>

namespace LikeHub {
    static LikesProgram::String ParseAction(const std::shared_ptr<Session>& session, const RpcMessage& request) {
        LikesProgram::String action = u"Unknown";
        try {
            const auto& metadata = request.MetaData();
            if (!metadata[u"action"].IsNull()) action = metadata[u"action"].AsString();
        } catch (const std::exception& e) {
            LogWarn(
                u"Hub service request action parse failed: error={} serviceIp={} serviceId={} requestId={}",
                e.what(), session ? LikesProgram::String(session->GetRemoteAddress().ToString()) : u"Unknown",
                session ? session->GetServiceId() : 0, request.Header().requestId
            );
        } catch (...) {
            LogWarn(
                u"Hub service request action parse failed: unknown error serviceIp={} serviceId={} requestId={}",
                session ? LikesProgram::String(session->GetRemoteAddress().ToString()) : u"Unknown",
                session ? session->GetServiceId() : 0, request.Header().requestId
            );
        }
        return action;
    }

    static void SendErrorResponse(const std::shared_ptr<Session>& session, uint64_t requestId, RpcMessage::Code code, const LikesProgram::String& message) {
        if (!session) return;
        LikesProgram::Configuration responseMeta;
        responseMeta[u"message"] = message;
        session->SendMessage(RpcMessage::Encode(RpcMessage::Type::Response, requestId, code, responseMeta, 0));
    }

    static void HandleHeartbeatResponse(const std::shared_ptr<Session>& session, const RpcMessage& response, double delayMs) {
        InstanceMetricsSnapshot metrics;
        metrics.delayMs = delayMs < 0.0 ? 0 : static_cast<uint64_t>(delayMs);

        try { // 解析负载信息
            const auto& metadata = response.MetaData();
            if (!metadata[u"metrics"].IsNull()) {
                const auto& metricsMeta = metadata[u"metrics"];
                if (!metricsMeta[u"cpuPct"].IsNull()) metrics.cpuPct = static_cast<uint32_t>(metricsMeta[u"cpuPct"].AsInt());
                if (!metricsMeta[u"memPct"].IsNull()) metrics.memPct = static_cast<uint32_t>(metricsMeta[u"memPct"].AsInt());
                if (!metricsMeta[u"stressIndicators"].IsNull()) metrics.stressIndicators = static_cast<uint64_t>(metricsMeta[u"stressIndicators"].AsInt64());
            }
        } catch (const std::exception& e) {
            LogWarn(
                u"Heartbeat response metrics parse failed: error={} serviceIp={} serviceId={} requestId={}",
                e.what(), session->GetRemoteAddress().ToString(), session->GetServiceId(), response.Header().requestId
            );
        } catch (...) {
            LogWarn(
                u"Heartbeat response metrics parse failed: unknown error serviceIp={} serviceId={} requestId={}",
                session->GetRemoteAddress().ToString(), session->GetServiceId(), response.Header().requestId
            );
        }

        // 做边界保护
        if (metrics.cpuPct > 100) metrics.cpuPct = 100;
        if (metrics.memPct > 100) metrics.memPct = 100;

        auto service = session->GetService();
        if (service) {
            // 更新负载信息
            service->UpdateLoad(metrics);
            // 触发节点更新事件
            ServiceRegistry::Instance().NotifyServiceChanged();
            service->SetStatus(LifecycleStatus::Active);
            service->SetHealthStatus(HealthStatus::Healthy);
        }

        LogDebug(
            u"Heartbeat response handled: serviceIp={} serviceId={} requestId={} cpuPct={} memPct={} stressIndicators={} delayMs={}",
            session->GetRemoteAddress().ToString(), session->GetServiceId(), response.Header().requestId, metrics.cpuPct, metrics.memPct, metrics.stressIndicators, metrics.delayMs
        );
    }

    void HandleDispatcher::HandleRequest(const std::shared_ptr<Session>& session, const RpcMessage& request) {
        if (!session) {
            LogWarn(u"Hub service request ignored: session is null");
            return;
        }

        if (request.Header().metaLength == 0) {
            LogWarn( // 请求缺少元数据
                u"Hub service request missing metadata: requestType={} serviceIp={} serviceId={} requestId={} metaLength={} bodyLength={}",
                RpcMessage::TypeToString(RpcMessage::UInt8ToType(request.Header().requestType)), session->GetRemoteAddress().ToString(),
                session->GetServiceId(), request.Header().requestId, request.Header().metaLength, request.Header().bodyLength
            );
            SendErrorResponse(session, request.Header().requestId, RpcMessage::Code::BadRequest, u"Missing request metadata");
            return;
        }

        LikesProgram::String action = ParseAction(session, request);
        if (action == u"Unknown" || action.Empty()) {
            LogWarn(
                u"Hub service request missing action: requestType={} serviceIp={} serviceId={} requestId={} metaLength={} bodyLength={}",
                RpcMessage::TypeToString(RpcMessage::UInt8ToType(request.Header().requestType)), session->GetRemoteAddress().ToString(),
                session->GetServiceId(), request.Header().requestId, request.Header().metaLength, request.Header().bodyLength
            );
            SendErrorResponse(session, request.Header().requestId, RpcMessage::Code::BadRequest, u"Missing request action");
            return;
        }

        LogDebug(
            u"Hub dispatch begin: eventName={} sourceServiceId={} sourceAddr={} requestId={} metaLength={} bodyLength={}",
            action,
            session->GetServiceId(),
            session->GetRemoteAddress().ToString(),
            request.Header().requestId,
            request.Header().metaLength,
            request.Header().bodyLength
        );

        EventTriggerResult result = EventDispatcher::Instance().Trigger(action.ToStdString(), request, session);

        LogDebug(
            u"Hub dispatch result: eventName={} listenType={} matchedCount={} targetCount={} successCount={} failedCount={} ignoredCount={} retryCount={} sourceServiceId={} requestId={}",
            action,
            ListenTypeToString(result.listenType),
            result.matchedCount,
            result.targetCount,
            result.successCount,
            result.failedCount,
            result.ignoredCount,
            result.retryCount,
            session->GetServiceId(),
            request.Header().requestId
        );

        if (!result.HasListener()) {
            LogWarn(
                u"Hub service no listeners for event: eventName={} serviceIp={} serviceId={} requestId={} metaLength={} bodyLength={}",
                action, session->GetRemoteAddress().ToString(), session->GetServiceId(), request.Header().requestId, request.Header().metaLength, request.Header().bodyLength
            );
            SendErrorResponse(session, request.Header().requestId, RpcMessage::Code::NotFound, u"No listeners for event");
            return;
        }

        if (!result.HasTarget()) {
            LogWarn(
                u"Hub service no dispatch target for event: eventName={} listenType={} matchedCount={} serviceIp={} serviceId={} requestId={}",
                action, ListenTypeToString(result.listenType), result.matchedCount, session->GetRemoteAddress().ToString(), session->GetServiceId(), request.Header().requestId
            );
            SendErrorResponse(session, request.Header().requestId, RpcMessage::Code::ServiceUnavailable, u"No available target for event");
            return;
        }

        LogDebug(
            u"Hub service event dispatched: eventName={} listenType={} matchedCount={} targetCount={} successCount={} failedCount={} serviceIp={} serviceId={} requestId={}",
            action, ListenTypeToString(result.listenType), result.matchedCount, result.targetCount, result.successCount, result.failedCount,
            session->GetRemoteAddress().ToString(), session->GetServiceId(), request.Header().requestId
        );

        // 给服务发起者回复
        if (result.listenType == ListenType::Service) {
            LikesProgram::Configuration responseMeta;
            responseMeta[u"message"] = u"OK";
            responseMeta[u"listenType"] = LikesProgram::String(ListenTypeToString(result.listenType));
            responseMeta[u"matchedCount"] = static_cast<int64_t>(result.matchedCount);
            responseMeta[u"targetCount"] = static_cast<int64_t>(result.targetCount);
            responseMeta[u"successCount"] = static_cast<int64_t>(result.successCount);
            responseMeta[u"failedCount"] = static_cast<int64_t>(result.failedCount);
            responseMeta[u"ignoredCount"] = static_cast<int64_t>(result.ignoredCount);
            responseMeta[u"retryCount"] = static_cast<int64_t>(result.retryCount);
            session->SendMessage(RpcMessage::Encode(
                RpcMessage::Type::Response, request.Header().requestId,
                result.failedCount == 0 ? RpcMessage::Code::OK : RpcMessage::Code::PartialContent,
                responseMeta, 0
            ));
        }
    }

    void HandleDispatcher::HandleResponse(const std::shared_ptr<Session>& session, const RpcMessage& response, const  RpcMessage& request, double delayMs) {
        if (!session) {
            LogWarn(u"Hub service response ignored: session is null");
            return;
        }

        // 获取返回代码
        int code = response.Header().code;
        
        // 优先从原始 request 中获取 Action
        LikesProgram::String action = ParseAction(session, request);
        // 如果原始 request 没有 action，再尝试从 response 中取
        if (action == u"Unknown" || action.Empty()) action = ParseAction(session, response);
        // 空值判断
        if (action == u"Unknown" || action.Empty()) {
            LogWarn(
                u"Hub service response missing action: responseType={} serviceIp={} serviceId={} requestId={} code={} delay={}",
                RpcMessage::TypeToString(RpcMessage::UInt8ToType(response.Header().requestType)), session->GetRemoteAddress().ToString(),
                session->GetServiceId(), response.Header().requestId, code, delayMs
            );
            return;
        }

        // 失败返回
        if (code != static_cast<int>(RpcMessage::Code::OK)) {
            LogWarn(
                u"Hub service request failed: code={} requestType={} serviceIp={} serviceId={} requestId={} metaLength={} bodyLength={} delay={}",
                code, RpcMessage::TypeToString(RpcMessage::UInt8ToType(response.Header().requestType)), session->GetRemoteAddress().ToString(),
                session->GetServiceId(), response.Header().requestId, response.Header().metaLength, response.Header().bodyLength,
                delayMs
            );
            return;
        }

        // 心跳信息返回
        if (action == u"Heartbeat") {
            HandleHeartbeatResponse(session, response, delayMs);
            return;
        }

        // 其他返回
        LogDebug(
            u"Hub service response succeeded: action={} requestType={} serviceIp={} serviceId={} requestId={} metaLength={} bodyLength={} delay={}",
            action, RpcMessage::TypeToString(RpcMessage::UInt8ToType(response.Header().requestType)), session->GetRemoteAddress().ToString(), session->GetServiceId(),
            response.Header().requestId, response.Header().metaLength, response.Header().bodyLength, delayMs
        );
    }

    void HandleDispatcher::HandleUnknown(const std::shared_ptr<Session>& session, const RpcMessage& request) {
        if (!session) {
            LogWarn(u"Hub service unknown request ignored: session is null");
            return;
        }
        LogWarn( // 未知请求类型
            u"Hub service unknown request type received: requestType={} serviceIp={} serviceId={} requestId={} metaLength={} bodyLength={}",
            RpcMessage::TypeToString(RpcMessage::UInt8ToType(request.Header().requestType)), session->GetRemoteAddress().ToString(),
            session->GetServiceId(), request.Header().requestId, request.Header().metaLength, request.Header().bodyLength
        );

        SendErrorResponse(session, request.Header().requestId, RpcMessage::Code::BadRequest, u"Unknown request type");
    }

    void HandleDispatcher::HandleTimeout(const std::shared_ptr<Session>& session, const RpcMessage& request) {
        if (!session) {
            LogWarn(u"Hub service timeout ignored: session is null");
            return;
        }
        LogWarn(
            u"Hub service request timeout handled: requestType={} serviceIp={} requestId={} metaLength={} bodyLength={}",
            RpcMessage::TypeToString(RpcMessage::UInt8ToType(request.Header().requestType)),
            session->GetRemoteAddress().ToString(),
            request.Header().requestId,
            request.Header().metaLength,
            request.Header().bodyLength
        );
    }
}
