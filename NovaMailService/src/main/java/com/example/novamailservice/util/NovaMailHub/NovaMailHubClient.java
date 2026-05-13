package com.example.novamailservice.util.NovaMailHub;

import com.example.novamailservice.util.NovaMailHub.Message.*;
import io.netty.bootstrap.Bootstrap;
import io.netty.channel.Channel;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelInitializer;
import io.netty.channel.ChannelOption;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.socket.SocketChannel;
import io.netty.channel.socket.nio.NioSocketChannel;
import io.netty.channel.nio.NioEventLoopGroup;
import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContextException;
import org.springframework.stereotype.Component;

import java.io.IOException;
import java.util.Map;
import java.util.concurrent.*;

@Component
public class NovaMailHubClient {
    private static final Logger logger = LoggerFactory.getLogger(NovaMailHubClient.class);

    private static final int DEFAULT_CONNECT_TIMEOUT_MS = 30000;
    private static final int DEFAULT_REQUEST_TIMEOUT_MS = 50000;

    private final NovaMailHubProperties novaMailHubProperties;
    private final EventLoopGroup group = new NioEventLoopGroup(1);
    private final ScheduledExecutorService timeoutExecutor =
            Executors.newSingleThreadScheduledExecutor(r -> {
                Thread thread = new Thread(r, "NovaMailHub-timeout");
                thread.setDaemon(true);
                return thread;
            });

    private final ConcurrentMap<Long, PendingRequest> pendingRequests = new ConcurrentHashMap<>();

    private volatile Channel channel;
    private final ClientHandler clientHandler;

    @Autowired
    public NovaMailHubClient(NovaMailHubProperties novaMailHubProperties, ClientHandler clientHandler) {
        this.novaMailHubProperties = novaMailHubProperties;
        this.clientHandler = clientHandler;

        // 避免在 ClientHandler 里反向构造依赖造成循环依赖
        this.clientHandler.bindClient(this);
    }

    @PostConstruct
    public void start() {
        Bootstrap bootstrap = new Bootstrap();

        bootstrap.group(group)
                .channel(NioSocketChannel.class)
                .option(ChannelOption.TCP_NODELAY, true)
                .option(ChannelOption.SO_KEEPALIVE, true)
                .option(ChannelOption.CONNECT_TIMEOUT_MILLIS, DEFAULT_CONNECT_TIMEOUT_MS)
                .handler(new ChannelInitializer<SocketChannel>() {
                    @Override
                    protected void initChannel(SocketChannel ch) {
                        ch.pipeline()
                                .addLast(new RpcMessageDecoder(16 * 1024 * 1024))
                                .addLast(new RpcMessageEncoder())
                                .addLast(clientHandler);
                    }
                });

        try {
            ChannelFuture connectFuture = bootstrap.connect(
                    novaMailHubProperties.getAddress(),
                    novaMailHubProperties.getPort()
            ).sync();

            this.channel = connectFuture.channel();

            this.channel.closeFuture().addListener(future -> {
                logger.warn("Hub channel closed");
                failAllPending(new IOException("Hub channel closed"));
            });

            logger.info(
                    "NovaMail Hub connected: {}:{}",
                    novaMailHubProperties.getAddress(),
                    novaMailHubProperties.getPort()
            );
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            group.shutdownGracefully();
            timeoutExecutor.shutdownNow();

            throw new ApplicationContextException("Connect to NovaMail Hub interrupted", e);
        } catch (Exception e) {
            group.shutdownGracefully();
            timeoutExecutor.shutdownNow();

            throw new ApplicationContextException("Connect to NovaMail Hub failed", e);
        }
    }

    /**
     * 发送普通消息。
     * 如果是 Request，会自动登记 pending 并执行超时控制。
     */
    public void send(RpcMessage message) {
        sendInternal(message, message.getType() == RpcType.REQUEST);
    }

    /**
     * 发送 Request，并返回 CompletableFuture 等待 Response。
     */
    public CompletableFuture<RpcMessage> sendRequest(RpcMessage message) {
        if (message.getType() != RpcType.REQUEST) {
            throw new IllegalArgumentException("sendRequest only accepts REQUEST message");
        }

        return sendInternal(message, true);
    }

    /**
     * 发送 Response，不登记 pending。
     */
    public void sendResponse(RpcMessage message) {
        if (message.getType() != RpcType.RESPONSE) {
            throw new IllegalArgumentException("sendResponse only accepts RESPONSE message");
        }

        sendInternal(message, false);
    }

    private CompletableFuture<RpcMessage> sendInternal(
            RpcMessage message,
            boolean needResponse
    ) {
        Channel current = this.channel;

        if (current == null || !current.isActive()) {
            throw new IllegalStateException("Hub channel is not active");
        }

        if (message.getType() == RpcType.REQUEST && message.getRequestId() == 0) {
            message.setRequestId(RequestIdGenerator.nextId());
        }

        long requestId = message.getRequestId();
        CompletableFuture<RpcMessage> future = new CompletableFuture<>();

        PendingRequest pending = null;

        if (needResponse) {
            pending = new PendingRequest(
                    requestId,
                    getAction(message),
                    message,
                    future,
                    System.currentTimeMillis()
            );

            PendingRequest old = pendingRequests.putIfAbsent(requestId, pending);
            if (old != null) {
                throw new IllegalStateException("Duplicate requestId: " + requestId);
            }

            int timeoutMs = resolveTimeoutMs(message);

            ScheduledFuture<?> timeoutFuture = timeoutExecutor.schedule(
                    () -> handleRequestTimeout(requestId),
                    timeoutMs,
                    TimeUnit.MILLISECONDS
            );

            pending.setTimeoutFuture(timeoutFuture);
        }

        PendingRequest finalPending = pending;

        current.writeAndFlush(message).addListener(writeFuture -> {
            if (!writeFuture.isSuccess()) {
                if (needResponse) {
                    PendingRequest removed = pendingRequests.remove(requestId);
                    if (removed != null) {
                        removed.cancelTimeout();
                        removed.future().completeExceptionally(writeFuture.cause());
                    }
                }

                logger.error(
                        "Send Hub message failed: action={} requestId={} type={}",
                        getAction(message),
                        requestId,
                        message.getType(),
                        writeFuture.cause()
                );

                return;
            }

            logger.debug(
                    "Send Hub message success: action={} requestId={} type={} code={} timeoutMs={} pending={}",
                    getAction(message),
                    requestId,
                    message.getType(),
                    message.getCode(),
                    resolveTimeoutMs(message),
                    finalPending != null
            );

            if (!needResponse) {
                future.complete(null);
            }
        });

        return future;
    }

    /**
     * ClientHandler 收到 Response 后调用此方法。
     */
    public boolean completeResponse(RpcMessage response) {
        long requestId = response.getRequestId();

        PendingRequest pending = pendingRequests.remove(requestId);

        if (pending == null) {
            logger.warn(
                    "Hub response received but pending not found: action={} requestId={} code={}",
                    getAction(response),
                    requestId,
                    response.getCode()
            );
            return false;
        }

        pending.cancelTimeout();

        long costMs = System.currentTimeMillis() - pending.createdAtMillis();

        logger.debug(
                "Hub response completed: requestAction={} responseAction={} requestId={} code={} costMs={}",
                pending.action(),
                getAction(response),
                requestId,
                response.getCode(),
                costMs
        );

        pending.future().complete(response);
        return true;
    }

    private void handleRequestTimeout(long requestId) {
        PendingRequest pending = pendingRequests.remove(requestId);

        if (pending == null) {
            logger.debug(
                    "Hub request timeout ignored because pending not found: requestId={}",
                    requestId
            );
            return;
        }

        pending.cancelTimeout();

        TimeoutException exception = new TimeoutException(
                "Hub request timeout: action=" + pending.action()
                        + ", requestId=" + requestId
        );

        pending.future().completeExceptionally(exception);

        logger.warn(
                "Hub request timeout: action={} requestId={} timeoutMs={}",
                pending.action(),
                requestId,
                resolveTimeoutMs(pending.message())
        );

        onRequestTimeout(pending);
    }

    /**
     * 统一超时处理入口。
     * 后续需要重连、重试、关闭通道，可以从这里扩展。
     */
    private void onRequestTimeout(PendingRequest pending) {
        String action = pending.action();

        if ("Login".equalsIgnoreCase(action)) {
            logger.warn("Login request timeout, close Hub channel");
            closeChannel();
            return;
        }

        if ("RegisterEvent".equalsIgnoreCase(action)) {
            logger.warn("RegisterEvent request timeout: requestId={}", pending.requestId());
            return;
        }

        logger.warn("Unhandled Hub request timeout: action={} requestId={}", action, pending.requestId());
    }

    private void failAllPending(Throwable cause) {
        for (Map.Entry<Long, PendingRequest> entry : pendingRequests.entrySet()) {
            PendingRequest pending = pendingRequests.remove(entry.getKey());

            if (pending == null) {
                continue;
            }

            pending.cancelTimeout();
            pending.future().completeExceptionally(cause);
        }
    }

    private int resolveTimeoutMs(RpcMessage message) {
        int timeoutMs = message.getTimeoutMs();

        if (timeoutMs > 0) {
            return timeoutMs;
        }

        return DEFAULT_REQUEST_TIMEOUT_MS;
    }

    private String getAction(RpcMessage message) {
        if (message == null || message.getMetadata() == null) {
            return "";
        }

        Object action = message.getMetadata().get("action");
        return action == null ? "" : String.valueOf(action);
    }

    private void closeChannel() {
        Channel current = this.channel;

        if (current != null) {
            current.close();
        }
    }

    @PreDestroy
    public void stop() {
        closeChannel();

        failAllPending(new IOException("NovaMailHubClient stopped"));

        timeoutExecutor.shutdownNow();
        group.shutdownGracefully();

        logger.info("NovaMail Hub client stopped");
    }

    private static final class PendingRequest {
        private final long requestId;
        private final String action;
        private final RpcMessage message;
        private final CompletableFuture<RpcMessage> future;
        private final long createdAtMillis;

        private volatile ScheduledFuture<?> timeoutFuture;

        private PendingRequest(
                long requestId,
                String action,
                RpcMessage message,
                CompletableFuture<RpcMessage> future,
                long createdAtMillis
        ) {
            this.requestId = requestId;
            this.action = action;
            this.message = message;
            this.future = future;
            this.createdAtMillis = createdAtMillis;
        }

        public long requestId() {
            return requestId;
        }

        public String action() {
            return action;
        }

        public RpcMessage message() {
            return message;
        }

        public CompletableFuture<RpcMessage> future() {
            return future;
        }

        public long createdAtMillis() {
            return createdAtMillis;
        }

        public void setTimeoutFuture(ScheduledFuture<?> timeoutFuture) {
            this.timeoutFuture = timeoutFuture;
        }

        public void cancelTimeout() {
            ScheduledFuture<?> future = this.timeoutFuture;

            if (future != null) {
                future.cancel(false);
            }
        }
    }
}