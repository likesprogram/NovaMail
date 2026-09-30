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
    private final Set<WebSocketSession> adminSessions = new CopyOnWriteArraySet<>();

    public void addSession(String account, WebSocketSession session) {
        accountSessions
                .computeIfAbsent(account, key -> new CopyOnWriteArraySet<>())
                .add(session);
        if (Boolean.TRUE.equals(session.getAttributes().get("isAdmin"))) {
            adminSessions.add(session);
        }
    }

    public void removeSession(String account, WebSocketSession session) {
        if (session != null) adminSessions.remove(session);

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

        sendToSessions(sessions, account, message);
    }

    public void sendToAdmins(String message) {
        if (adminSessions.isEmpty()) return;

        for (WebSocketSession session : adminSessions) {
            if (session == null || !session.isOpen()) {
                adminSessions.remove(session);
                continue;
            }

            try {
                synchronized (session) {
                    session.sendMessage(new TextMessage(message));
                }
            } catch (IOException e) {
                adminSessions.remove(session);
            }
        }
    }

    public boolean isOnline(String account) {
        Set<WebSocketSession> sessions = accountSessions.get(account);
        return sessions != null && !sessions.isEmpty();
    }

    private void sendToSessions(Set<WebSocketSession> sessions, String account, String message) {
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
}
