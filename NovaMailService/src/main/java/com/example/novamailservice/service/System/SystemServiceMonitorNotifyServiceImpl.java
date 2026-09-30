package com.example.novamailservice.service.System;

import com.example.novamailservice.config.WebSocket.MailWebSocketSessionManager;
import com.example.novamailservice.dto.System.SystemServiceMonitorInfo;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.HashMap;
import java.util.Map;

@Service
public class SystemServiceMonitorNotifyServiceImpl implements SystemServiceMonitorNotifyService {
    private final MailWebSocketSessionManager sessionManager;
    private final ObjectMapper objectMapper;

    @Autowired
    public SystemServiceMonitorNotifyServiceImpl(MailWebSocketSessionManager sessionManager, ObjectMapper objectMapper) {
        this.sessionManager = sessionManager;
        this.objectMapper = objectMapper;
    }

    @Override
    public void notifyServiceChanged(SystemServiceMonitorInfo service) {
        if (service == null) return;

        try {
            Map<String, Object> message = new HashMap<>();
            message.put("type", "SERVICE_CHANGED");
            message.put("data", service);
            message.put("timestamp", System.currentTimeMillis());

            sessionManager.sendToAdmins(objectMapper.writeValueAsString(message));
        } catch (Exception ignored) {
            // 服务监控通知失败不影响 Hub 消息响应。
        }
    }
}
