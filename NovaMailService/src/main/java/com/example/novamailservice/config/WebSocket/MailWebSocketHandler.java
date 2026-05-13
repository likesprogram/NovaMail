package com.example.novamailservice.config.WebSocket;

import org.jetbrains.annotations.NotNull;
import org.springframework.stereotype.Component;
import org.springframework.web.socket.CloseStatus;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;

@Component
public class MailWebSocketHandler extends TextWebSocketHandler {
    private final MailWebSocketSessionManager sessionManager;

    public MailWebSocketHandler(MailWebSocketSessionManager sessionManager) {
        this.sessionManager = sessionManager;
    }

    @Override
    public void afterConnectionEstablished(WebSocketSession session) throws Exception {
        Long userId = (Long) session.getAttributes().get("userId");
        String account = (String) session.getAttributes().get("account");

        if (userId == null || account == null || account.isBlank()) {
            session.close(CloseStatus.NOT_ACCEPTABLE.withReason("Unauthorized"));
            return;
        }

        sessionManager.addSession(account, session);
    }

    @Override
    protected void handleTextMessage(@NotNull WebSocketSession session, @NotNull TextMessage message) throws Exception {
        Long userId = (Long) session.getAttributes().get("userId");
        String account = (String) session.getAttributes().get("account");

        if (userId == null || account == null) session.close(CloseStatus.NOT_ACCEPTABLE.withReason("Unauthorized"));
    }

    @Override
    public void afterConnectionClosed(@NotNull WebSocketSession session, @NotNull CloseStatus status) {
        removeSession(session);
    }

    @Override
    public void handleTransportError(@NotNull WebSocketSession session, @NotNull Throwable exception) throws Exception {
        removeSession(session);
        if (session.isOpen()) session.close(CloseStatus.SERVER_ERROR);
    }

    private void removeSession(WebSocketSession session) {
        String account = (String) session.getAttributes().get("account");
        if (account != null && !account.isBlank()) sessionManager.removeSession(account, session);
    }
}