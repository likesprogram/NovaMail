package com.example.novamailservice.service.Mail.Impl;

import com.example.novamailservice.config.WebSocket.MailWebSocketSessionManager;
import com.example.novamailservice.service.Mail.MailNotifyService;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.HashMap;
import java.util.Map;

@Service
public class MailNotifyServiceImpl implements MailNotifyService {
    private final MailWebSocketSessionManager sessionManager;
    private final ObjectMapper objectMapper;

    @Autowired
    public MailNotifyServiceImpl(MailWebSocketSessionManager sessionManager, ObjectMapper objectMapper) {
        this.sessionManager = sessionManager;
        this.objectMapper = objectMapper;
    }

    @Override
    public void notifyNewEmail(String account, Long mappingId, Long mailBoxId, Long mailId, Long folderId) {
        Map<String, Object> data = new HashMap<>();
        data.put("mappingId", mappingId);
        data.put("mailBoxId", mailBoxId);
        data.put("mailId", mailId);
        data.put("folderId", folderId);

        send(account, "NEW_EMAIL", data);
    }

    @Override
    public void notifyMailMoved(String account, Long mappingId, Long fromFolderId, Long toFolderId) {
        Map<String, Object> data = new HashMap<>();
        data.put("mappingId", mappingId);
        data.put("fromFolderId", fromFolderId);
        data.put("toFolderId", toFolderId);

        send(account, "MAIL_MOVED", data);
    }

    @Override
    public void notifyMailStatusChanged(String account, Long mappingId, String status) {
        Map<String, Object> data = new HashMap<>();
        data.put("mappingId", mappingId);
        data.put("status", status);

        send(account, "MAIL_STATUS_CHANGED", data);
    }

    private void send(String account, String type, Map<String, Object> data) {
        if (account == null || account.isBlank()) return;

        try {
            Map<String, Object> message = new HashMap<>();
            message.put("type", type);
            message.put("data", data);
            message.put("timestamp", System.currentTimeMillis());

            String json = objectMapper.writeValueAsString(message);

            sessionManager.sendToAccount(account, json);
        } catch (Exception e) {
            // WebSocket 通知失败不应该影响邮件主流程
            e.printStackTrace();
        }
    }
}
