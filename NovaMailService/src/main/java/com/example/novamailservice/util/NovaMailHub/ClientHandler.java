package com.example.novamailservice.util.NovaMailHub;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.Mail.MailNotifyService;
import com.example.novamailservice.service.System.SystemServiceMonitorNotifyService;
import com.example.novamailservice.service.User.UsersService;
import com.example.novamailservice.util.NovaMailHub.Message.*;
import io.netty.channel.ChannelHandler;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelOutboundBuffer;
import io.netty.channel.SimpleChannelInboundHandler;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import java.util.HashMap;
import java.util.Map;

@Component
@ChannelHandler.Sharable
public class ClientHandler extends SimpleChannelInboundHandler<RpcMessage> {
    private static final Logger logger = LoggerFactory.getLogger(ClientHandler.class);

    private final Session session;
    private final MailNotifyService mailNotifyService;
    private final UsersService usersService;
    private final ServiceMonitorStore serviceMonitorStore;
    private final SystemServiceMonitorNotifyService systemServiceMonitorNotifyService;

    /**
     * 由 NovaMailHubClient 启动时绑定。
     * 不建议构造器直接注入 NovaMailHubClient，否则容易形成循环依赖。
     */
    private volatile NovaMailHubClient hubClient;

    @Autowired
    public ClientHandler(Session session, MailNotifyService mailNotifyService, UsersService usersService, ServiceMonitorStore serviceMonitorStore, SystemServiceMonitorNotifyService systemServiceMonitorNotifyService) {
        this.session = session;
        this.mailNotifyService = mailNotifyService;
        this.usersService = usersService;
        this.serviceMonitorStore = serviceMonitorStore;
        this.systemServiceMonitorNotifyService = systemServiceMonitorNotifyService;
    }

    public void bindClient(NovaMailHubClient hubClient) {
        this.hubClient = hubClient;
    }

    @Override
    protected void channelRead0(ChannelHandlerContext ctx, RpcMessage msg) {
        if (msg.getType() == RpcType.RESPONSE) {
            handleResponse(msg);
            return;
        }

        if (msg.getType() == RpcType.REQUEST) {
            handleRequest(ctx, msg);
            return;
        }

        handleUnknown(msg);
    }
    private int clampPct(int value) {
        if (value < 0) return 0;
        if (value > 100) return 100;
        return value;
    }
    private int getCpuPct() {
        try {
            java.lang.management.OperatingSystemMXBean baseBean =
                    java.lang.management.ManagementFactory.getOperatingSystemMXBean();

            if (baseBean instanceof com.sun.management.OperatingSystemMXBean osBean) {
                double load = osBean.getProcessCpuLoad();

                if (load < 0) return 0;

                return clampPct((int) Math.round(load * 100));
            }
        } catch (Exception ignored) { }

        return 0;
    }

    private int getMemPct() {
        try {
            Runtime runtime = Runtime.getRuntime();

            long max = runtime.maxMemory();
            long total = runtime.totalMemory();
            long free = runtime.freeMemory();

            long used = total - free;

            if (max <= 0) return 0;

            return clampPct((int) Math.round((used * 100.0) / max));
        } catch (Exception ignored) {
            return 0;
        }
    }

    private long getStressIndicators(ChannelHandlerContext ctx) {
        try {
            ChannelOutboundBuffer buffer = ctx.channel().unsafe().outboundBuffer();
            if (buffer == null) return 0L;

            long pendingBytes = Math.max(0L, buffer.totalPendingWriteBytes());

            int highWaterMark = ctx.channel().config().getWriteBufferHighWaterMark();
            if (highWaterMark <= 0) return 0L;

            long stressPct = Math.round((pendingBytes * 100.0) / highWaterMark);

            if (stressPct < 0) return 0L;
            if (stressPct > 100) return 100L;
            return stressPct;
        } catch (Exception ignored) {
            return 0L;
        }
    }

    private Map<String, Object> buildMetrics(ChannelHandlerContext ctx) {
        Map<String, Object> metrics = new HashMap<>();

        metrics.put("cpuPct", getCpuPct());
        metrics.put("memPct", getMemPct());

        // 压力指标: 当前 Channel 是否活跃 + 待发送字节数估算。
        metrics.put("stressIndicators", getStressIndicators(ctx));

        return metrics;
    }

    private void handleResponse(RpcMessage response) {
        NovaMailHubClient client = this.hubClient;

        if (client == null) {
            logger.warn(
                    "Hub response ignored because client is not bound: action={} requestId={} code={}",
                    getAction(response),
                    response.getRequestId(),
                    response.getCode()
            );
            return;
        }

        boolean completed = client.completeResponse(response);

        if (!completed) {
            logger.warn(
                    "Hub response not matched pending request: action={} requestId={} code={} metadata={}",
                    getAction(response),
                    response.getRequestId(),
                    response.getCode(),
                    response.getMetadata()
            );
        }
    }

    private void handleRequest(ChannelHandlerContext ctx, RpcMessage request) {
        String action = getAction(request);

        switch (action) {
            case "GetServiceInfo" -> handleGetServiceInfo(ctx, request);
            case "Heartbeat" -> handleHeartbeat(ctx, request);
            case "ServiceChanged" -> handleServiceChanged(ctx, request);
            case "NewEmailDelivery" -> handleNewEmailDelivery(ctx, request);
            default -> handleUnknown(request);
        }
    }

    private void handleGetServiceInfo(ChannelHandlerContext ctx, RpcMessage request) {
        logger.info("Received GetServiceInfo request requestId={}", request.getRequestId());

        Object serviceIdValue = request.getMetadata().get("serviceId");

        if (serviceIdValue == null || String.valueOf(serviceIdValue).isBlank()) {
            logger.warn(
                    "Hub GetServiceInfo request missing serviceId: requestId={} metadata={}",
                    request.getRequestId(),
                    request.getMetadata()
            );

            sendResponse(
                    ctx,
                    request,
                    RpcCode.BAD_REQUEST,
                    "Missing serviceId",
                    null
            );
            return;
        }

        String serviceId = String.valueOf(serviceIdValue);
        session.setServiceId(serviceId);

        Map<String, Object> responseMeta = new HashMap<>();
        responseMeta.put("action", "GetServiceInfo");
        responseMeta.put("serviceId", serviceId);
        responseMeta.put("serviceName", "NovaMailWeb");
        responseMeta.put("message", "GetServiceInfo success");
        responseMeta.put("code", RpcCode.OK.value());

        RpcMessage response = RpcCodec.encode(
                RpcType.RESPONSE,
                request.getRequestId(),
                RpcCode.OK,
                responseMeta,
                0,
                request.getVersion()
        );

        ctx.writeAndFlush(response).addListener(future -> {
            if (!future.isSuccess()) {
                logger.error(
                        "Send GetServiceInfo response failed requestId={}",
                        request.getRequestId(),
                        future.cause()
                );
                ctx.close();
                return;
            }

            logger.info(
                    "Send GetServiceInfo response success requestId={} serviceId={}",
                    request.getRequestId(),
                    serviceId
            );

            sendLogin(ctx, serviceId);
        });
    }
    private String asString(Object value) {
        if (value == null) return null;
        return String.valueOf(value);
    }

    private Long asLong(Object value) {
        if (value == null) return null;
        if (value instanceof Number number) return number.longValue();
        try {
            return Long.parseLong(String.valueOf(value));
        } catch (NumberFormatException e) {
            return null;
        }
    }

    private Double asDouble(Object value) {
        if (value == null) return null;
        if (value instanceof Number number) return number.doubleValue();
        try {
            return Double.parseDouble(String.valueOf(value));
        } catch (NumberFormatException e) {
            return null;
        }
    }

    private Map<String, Object> asMap(Object value) {
        if (!(value instanceof Map<?, ?> rawMap)) return null;

        Map<String, Object> result = new HashMap<>();
        rawMap.forEach((key, mapValue) -> {
            if (key != null) result.put(String.valueOf(key), mapValue);
        });
        return result;
    }

    private Object firstMetadataValue(Map<String, Object> primary, Map<String, Object> fallback, String key) {
        Object value = primary == null ? null : primary.get(key);
        if (value != null) return value;
        return fallback == null ? null : fallback.get(key);
    }

    private void notifyNewEmailByWebSocket(RpcMessage request) {
        Map<String, Object> meta = request.getMetadata();

        if (meta == null || meta.isEmpty()) {
            logger.warn("NewEmailDelivery metadata is empty requestId={}", request.getRequestId());
            return;
        }

        Long mappingId = asLong(meta.get("mappingId"));
        Long mailBoxId = asLong(meta.get("mailBoxId"));
        Long mailId = asLong(meta.get("mailId"));
        Long folderId = asLong(meta.get("folderId"));

        if (mappingId == null || mailBoxId == null || mailId == null || folderId == null) {
            logger.warn(
                "NewEmailDelivery metadata missing required field requestId={} mappingId={} mailBoxId={} mailId={} folderId={}",
                request.getRequestId(), mappingId, mailBoxId, mailId, folderId
            );
            return;
        }

        logger.info(
            "NewEmailDelivery parsed requestId={} mappingId={} mailBoxId={} mailId={} folderId={}",
            request.getRequestId(), mappingId, mailBoxId, mailId, folderId
        );

        // 当前协议没有 account，不能从 metadata 里取 account。
        // 如果 WebSocketSessionManager 是按 account 存储连接，这里必须通过 mailBoxId 查 account。
        ResponseResult<Users> result = usersService.getUserByMailBoxId(mailBoxId);

        if (!result.isSuccess()) {
            logger.warn("NewEmailDelivery cannot resolve account requestId={} mailBoxId={}", request.getRequestId(), mailBoxId);
            return;
        }

        mailNotifyService.notifyNewEmail(result.getData().getAccount(), mappingId, mailBoxId, mailId, folderId);
    }

    private void sendLogin(ChannelHandlerContext ctx, String serviceId) {
        NovaMailHubClient client = this.hubClient;

        if (client == null) {
            logger.error("Cannot send Login request because Hub client is not bound");
            ctx.close();
            return;
        }

        Map<String, Object> loginMeta = new HashMap<>();
        loginMeta.put("action", "Login");
        loginMeta.put("serviceName", "NovaMailWeb");
        loginMeta.put("userName", "nov_amail_hub_server");
        loginMeta.put("password", "nov_amail_hub_server");

        RpcMessage loginRequest = RpcCodec.encode(
                RpcType.REQUEST,
                RequestIdGenerator.nextId(),
                RpcCode.NONE,
                loginMeta,
                5000,
                1
        );

        client.sendRequest(loginRequest).whenComplete((response, ex) -> {
            if (ex != null) {
                logger.error(
                        "Login request failed serviceId={} requestId={}",
                        serviceId,
                        loginRequest.getRequestId(),
                        ex
                );
                ctx.close();
                return;
            }

            if (response.getCode() == RpcCode.OK) {
                logger.info(
                        "Login success serviceId={} requestId={} responseMetadata={}",
                        serviceId,
                        response.getRequestId(),
                        response.getMetadata()
                );
                registerServiceChangedEvent(ctx, serviceId);
                registerNewEmailDeliveryEvent(ctx, serviceId);
                return;
            }

            logger.warn(
                    "Login rejected serviceId={} requestId={} code={} metadata={}",
                    serviceId,
                    response.getRequestId(),
                    response.getCode(),
                    response.getMetadata()
            );

            ctx.close();
        });
    }

    private void handleHeartbeat(ChannelHandlerContext ctx, RpcMessage request) {
        long receivedAt = System.currentTimeMillis();
        String serviceId;

        try {
            serviceId = session.requireServiceId();
        } catch (Exception e) {
            logger.warn(
                    "Heartbeat rejected because serviceId is missing requestId={}",
                    request.getRequestId(),
                    e
            );

            sendResponse(
                    ctx,
                    request,
                    RpcCode.FORBIDDEN,
                    "Service not initialized",
                    null
            );

            ctx.close();
            return;
        }

        Map<String, Object> metrics = buildMetrics(ctx);
        Map<String, Object> responseMeta = new HashMap<>();
        responseMeta.put("action", "Heartbeat");
        responseMeta.put("message", "Heartbeat OK");
        responseMeta.put("code", RpcCode.OK.value());
        responseMeta.put("metrics", metrics);

        RpcMessage response = RpcCodec.encode(
                RpcType.RESPONSE,
                request.getRequestId(),
                RpcCode.OK,
                responseMeta,
                0,
                request.getVersion()
        );

        ctx.writeAndFlush(response).addListener(future -> {
            if (!future.isSuccess()) {
                logger.error(
                        "Send Heartbeat response failed requestId={}",
                        request.getRequestId(),
                        future.cause()
                );
                ctx.close();
                return;
            }

            logger.debug(
                    "Send Heartbeat response success requestId={} serviceId={}",
                    request.getRequestId(),
                    serviceId
            );

            notifyCurrentNodeMonitor(ctx, serviceId, System.currentTimeMillis() - receivedAt);
        });
    }

    private void notifyCurrentNodeMonitor(ChannelHandlerContext ctx, String serviceId, long delayMs) {
        if (serviceId == null || serviceId.isBlank()) return;
        if (ctx == null || !ctx.channel().isActive()) return;

        try {
            systemServiceMonitorNotifyService.notifyServiceChanged(
                    serviceMonitorStore.updateCurrentNodeHeartbeat(serviceId, buildMetrics(ctx), delayMs)
            );
        } catch (Exception e) {
            logger.warn("Push current node monitor snapshot failed serviceId={}", serviceId, e);
        }
    }

    private void handleServiceChanged(ChannelHandlerContext ctx, RpcMessage request) {
        String serviceId;

        try {
            serviceId = session.requireServiceId();
        } catch (Exception e) {
            logger.warn(
                    "ServiceChanged rejected because serviceId is missing requestId={}",
                    request.getRequestId(),
                    e
            );

            sendResponse(
                    ctx,
                    request,
                    RpcCode.FORBIDDEN,
                    "Service not initialized",
                    null
            );

            ctx.close();
            return;
        }

        Map<String, Object> metadata = request.getMetadata();
        Map<String, Object> serviceMeta = metadata == null ? null : asMap(metadata.get("service"));
        Map<String, Object> changedServiceMeta = serviceMeta == null ? metadata : serviceMeta;
        Map<String, Object> loadMeta = asMap(firstMetadataValue(changedServiceMeta, metadata, "load"));

        Long changedServiceId = asLong(firstMetadataValue(changedServiceMeta, metadata, "serviceId"));
        String changedServiceName = asString(firstMetadataValue(changedServiceMeta, metadata, "serviceName"));
        String changedServiceAddress = asString(firstMetadataValue(changedServiceMeta, metadata, "serviceAddress"));
        Long lifecycle = asLong(firstMetadataValue(changedServiceMeta, metadata, "lifecycle"));
        Long health = asLong(firstMetadataValue(changedServiceMeta, metadata, "health"));

        if (changedServiceId == null || changedServiceName == null || changedServiceAddress == null) {
            logger.warn(
                    "ServiceChanged metadata missing service snapshot fields requestId={} changedServiceId={} changedServiceName={} changedServiceAddress={} metadata={}",
                    request.getRequestId(),
                    changedServiceId,
                    changedServiceName,
                    changedServiceAddress,
                    metadata
            );
        }

        logger.debug(
                "NovaMailHub ServiceChanged received requestId={} listenerServiceId={} changedServiceId={} changedServiceName={} changedServiceAddress={} lifecycle={} health={} cpuPct={} memPct={} stressIndicators={} delayMs={} cpu={} mem={} stress={} lat={} load={} effectiveLoad={}",
                request.getRequestId(),
                serviceId,
                changedServiceId,
                changedServiceName,
                changedServiceAddress,
                lifecycle,
                health,
                loadMeta == null ? null : asDouble(loadMeta.get("cpuPct")),
                loadMeta == null ? null : asDouble(loadMeta.get("memPct")),
                loadMeta == null ? null : asDouble(loadMeta.get("stressIndicators")),
                loadMeta == null ? null : asDouble(loadMeta.get("delayMs")),
                loadMeta == null ? null : asDouble(loadMeta.get("cpu")),
                loadMeta == null ? null : asDouble(loadMeta.get("mem")),
                loadMeta == null ? null : asDouble(loadMeta.get("stress")),
                loadMeta == null ? null : asDouble(loadMeta.get("lat")),
                loadMeta == null ? null : asDouble(loadMeta.get("load")),
                loadMeta == null ? null : asDouble(loadMeta.get("effectiveLoad"))
        );

        systemServiceMonitorNotifyService.notifyServiceChanged(serviceMonitorStore.upsert(metadata));
        notifyCurrentNodeMonitor(ctx, serviceId, -1L);

        Map<String, Object> metrics = buildMetrics(ctx);
        Map<String, Object> responseMeta = new HashMap<>();
        responseMeta.put("action", "ServiceChanged");
        responseMeta.put("serviceId", serviceId);
        responseMeta.put("message", "ServiceChanged OK");
        responseMeta.put("code", RpcCode.OK.value());
        responseMeta.put("metrics", metrics);

        RpcMessage response = RpcCodec.encode(
                RpcType.RESPONSE,
                request.getRequestId(),
                RpcCode.OK,
                responseMeta,
                0,
                request.getVersion()
        );

        ctx.writeAndFlush(response).addListener(future -> {
            if (!future.isSuccess()) {
                logger.error(
                        "Send ServiceChanged response failed requestId={}",
                        request.getRequestId(),
                        future.cause()
                );
                ctx.close();
                return;
            }

            logger.debug(
                    "Send ServiceChanged response success requestId={} serviceId={}",
                    request.getRequestId(),
                    serviceId
            );
        });
    }

    private void handleNewEmailDelivery(ChannelHandlerContext ctx, RpcMessage request) {
        String serviceId;

        try {
            serviceId = session.requireServiceId();
        } catch (Exception e) {
            logger.warn(
                    "NewEmailDelivery rejected because serviceId is missing requestId={}",
                    request.getRequestId(),
                    e
            );

            sendResponse(
                    ctx,
                    request,
                    RpcCode.FORBIDDEN,
                    "Service not initialized",
                    null
            );

            ctx.close();
            return;
        }

        Map<String, Object> metrics = buildMetrics(ctx);
        Map<String, Object> responseMeta = new HashMap<>();
        responseMeta.put("action", "NewEmailDelivery");
        responseMeta.put("message", "NewEmailDelivery OK");
        responseMeta.put("code", RpcCode.OK.value());
        responseMeta.put("metrics", metrics);

        RpcMessage response = RpcCodec.encode(
                RpcType.RESPONSE,
                request.getRequestId(),
                RpcCode.OK,
                responseMeta,
                0,
                request.getVersion()
        );

        ctx.writeAndFlush(response).addListener(future -> {
            if (!future.isSuccess()) {
                logger.error(
                        "Send NewEmailDelivery response failed requestId={}",
                        request.getRequestId(),
                        future.cause()
                );
                ctx.close();
                return;
            }

            logger.debug(
                    "Send NewEmailDelivery response success requestId={} serviceId={}",
                    request.getRequestId(),
                    serviceId
            );
            try {
                notifyNewEmailByWebSocket(request);
            } catch (Exception e) {
                logger.warn(
                        "Notify new email by websocket failed requestId={}",
                        request.getRequestId(),
                        e
                );
            }
        });


        logger.info("NovaMailHub message: OnNewEmailDelivery");
    }

    private void registerServiceChangedEvent(ChannelHandlerContext ctx, String serviceId) {
        NovaMailHubClient client = this.hubClient;

        if (client == null) {
            logger.error("Cannot register ServiceChanged event because Hub client is not bound");
            ctx.close();
            return;
        }

        Map<String, Object> registerMeta = new HashMap<>();
        registerMeta.put("action", "RegisterEvent");

        // 要监听的 Hub 事件名
        registerMeta.put("eventName", "ServiceChanged");

        /*
         * C++:
         * ListenType::Event   = 0
         * ListenType::Service = 1
         *
         * ServiceChanged 是广播通知，必须注册为 Service。
         */
        registerMeta.put("listenType", 1);

        RpcMessage registerRequest = RpcCodec.encode(
                RpcType.REQUEST,
                RequestIdGenerator.nextId(),
                RpcCode.NONE,
                registerMeta,
                5000,
                1
        );

        client.sendRequest(registerRequest).whenComplete((response, ex) -> {
            if (ex != null) {
                logger.error(
                        "Register ServiceChanged event failed serviceId={} requestId={}",
                        serviceId,
                        registerRequest.getRequestId(),
                        ex
                );
                ctx.close();
                return;
            }

            if (response.getCode() == RpcCode.OK) {
                logger.info(
                        "Register ServiceChanged event success serviceId={} requestId={} responseMetadata={}",
                        serviceId,
                        response.getRequestId(),
                        response.getMetadata()
                );
                return;
            }

            logger.warn(
                    "Register ServiceChanged event rejected serviceId={} requestId={} code={} metadata={}",
                    serviceId,
                    response.getRequestId(),
                    response.getCode(),
                    response.getMetadata()
            );

            ctx.close();
        });
    }

    private void registerNewEmailDeliveryEvent(ChannelHandlerContext ctx, String serviceId) {
        NovaMailHubClient client = this.hubClient;

        if (client == null) {
            logger.error("Cannot register NewEmailDelivery event because Hub client is not bound");
            ctx.close();
            return;
        }

        Map<String, Object> registerMeta = new HashMap<>();
        registerMeta.put("action", "RegisterEvent");

        // 要监听的 Hub 事件名
        registerMeta.put("eventName", "NewEmailDelivery");

        /*
         * C++:
         * ListenType::Event   = 0
         * ListenType::Service = 1
         *
         * ServiceChanged 是广播通知，必须注册为 Service。
         */
        registerMeta.put("listenType", 1);

        RpcMessage registerRequest = RpcCodec.encode(
                RpcType.REQUEST,
                RequestIdGenerator.nextId(),
                RpcCode.NONE,
                registerMeta,
                5000,
                1
        );

        client.sendRequest(registerRequest).whenComplete((response, ex) -> {
            if (ex != null) {
                logger.error(
                        "Register NewEmailDelivery event failed serviceId={} requestId={}",
                        serviceId,
                        registerRequest.getRequestId(),
                        ex
                );
                ctx.close();
                return;
            }

            if (response.getCode() == RpcCode.OK) {
                logger.info(
                        "Register NewEmailDelivery event success serviceId={} requestId={} responseMetadata={}",
                        serviceId,
                        response.getRequestId(),
                        response.getMetadata()
                );
                return;
            }

            logger.warn(
                    "Register NewEmailDelivery event rejected serviceId={} requestId={} code={} metadata={}",
                    serviceId,
                    response.getRequestId(),
                    response.getCode(),
                    response.getMetadata()
            );

            ctx.close();
        });
    }

    private void sendResponse(
            ChannelHandlerContext ctx,
            RpcMessage request,
            RpcCode code,
            String message,
            Map<String, Object> extraMeta
    ) {
        Map<String, Object> responseMeta = new HashMap<>();

        String action = getAction(request);
        if (!action.isBlank()) {
            responseMeta.put("action", action);
        }

        responseMeta.put("code", code.value());
        responseMeta.put("message", message);

        if (session.hasServiceId()) {
            responseMeta.put("serviceId", session.requireServiceId());
        }

        if (extraMeta != null && !extraMeta.isEmpty()) {
            responseMeta.putAll(extraMeta);
        }

        RpcMessage response = RpcCodec.encode(
                RpcType.RESPONSE,
                request.getRequestId(),
                code,
                responseMeta,
                0,
                request.getVersion()
        );

        ctx.writeAndFlush(response).addListener(future -> {
            if (!future.isSuccess()) {
                logger.error(
                        "Send Hub response failed action={} requestId={} code={}",
                        action,
                        request.getRequestId(),
                        code,
                        future.cause()
                );
                ctx.close();
            }
        });
    }

    private void handleUnknown(RpcMessage msg) {
        logger.warn(
                "Unknown Hub message: type={} action={} requestId={} code={} metadata={}",
                msg.getType(),
                getAction(msg),
                msg.getRequestId(),
                msg.getCode(),
                msg.getMetadata()
        );
    }

    private String getAction(RpcMessage msg) {
        if (msg == null || msg.getMetadata() == null) {
            return "";
        }

        Object action = msg.getMetadata().get("action");
        return action == null ? "" : String.valueOf(action);
    }

    @Override
    public void channelInactive(ChannelHandlerContext ctx) {
        logger.info("Disconnected from Hub");
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, Throwable cause) {
        logger.error("ExceptionCaught in Hub client", cause);
        ctx.close();
    }
}
