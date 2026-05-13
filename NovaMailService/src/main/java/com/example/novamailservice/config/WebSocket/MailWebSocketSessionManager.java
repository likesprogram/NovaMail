package com.example.novamailservice.config.WebSocket;

import org.springframework.stereotype.Component;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;
import java.io.IOException;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.CopyOnWriteArraySet;

@Component
public class MailWebSocketSessionManager {

    private final ConcurrentMap<String, Set<WebSocketSession>> accountSessions = new ConcurrentHashMap<>();

    public void addSession(String account, WebSocketSession session) {
        accountSessions
                .computeIfAbsent(account, key -> new CopyOnWriteArraySet<>())
                .add(session);
    }

    public void removeSession(String account, WebSocketSession session) {
        Set<WebSocketSession> sessions = accountSessions.get(account);
        if (sessions == null) return;

        sessions.remove(session);

        if (sessions.isEmpty()) accountSessions.remove(account);
    }

    public void sendToAccount(String account, String message) {
        Set<WebSocketSession> sessions = accountSessions.get(account);
        if (sessions == null || sessions.isEmpty()) {
            return;
        }

        for (WebSocketSession session : sessions) {
            if (session == null || !session.isOpen()) {
                removeSession(account, session);
                continue;
            }

            try {
                synchronized (session) {
                    session.sendMessage(new TextMessage(message));
                }
            } catch (IOException e) {
                removeSession(account, session);
            }
        }
    }

    public boolean isOnline(String account) {
        Set<WebSocketSession> sessions = accountSessions.get(account);
        return sessions != null && !sessions.isEmpty();
    }
}