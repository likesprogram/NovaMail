package com.example.novamailservice.config.WebSocket;

import org.springframework.context.annotation.Configuration;
import org.springframework.web.socket.config.annotation.EnableWebSocket;
import org.springframework.web.socket.config.annotation.WebSocketConfigurer;
import org.springframework.web.socket.config.annotation.WebSocketHandlerRegistry;

@Configuration
@EnableWebSocket
public class WebSocketConfig implements WebSocketConfigurer {

    private final MailWebSocketHandler mailWebSocketHandler;
    private final WebSocketAuthInterceptor webSocketAuthInterceptor;

    public WebSocketConfig(MailWebSocketHandler mailWebSocketHandler, WebSocketAuthInterceptor webSocketAuthInterceptor) {
        this.mailWebSocketHandler = mailWebSocketHandler;
        this.webSocketAuthInterceptor = webSocketAuthInterceptor;
    }

    @Override
    public void registerWebSocketHandlers(WebSocketHandlerRegistry registry) {
        registry.addHandler(mailWebSocketHandler, "/ws/mail")
                .addInterceptors(webSocketAuthInterceptor)
                .setAllowedOriginPatterns("*");
    }
}