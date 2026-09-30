package com.example.novamailservice.config.WebSocket;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.User.UsersService;
import org.jetbrains.annotations.NotNull;
import org.springframework.http.HttpStatus;
import org.springframework.http.server.ServerHttpRequest;
import org.springframework.http.server.ServerHttpResponse;
import org.springframework.stereotype.Component;
import org.springframework.web.socket.WebSocketHandler;
import org.springframework.web.socket.server.HandshakeInterceptor;

import java.util.Map;

@Component
public class WebSocketAuthInterceptor implements HandshakeInterceptor {
    private final UsersService usersService;
    public WebSocketAuthInterceptor(UsersService usersService) {
        this.usersService = usersService;
    }

    @Override
    public boolean beforeHandshake(@NotNull ServerHttpRequest request, @NotNull ServerHttpResponse response, @NotNull WebSocketHandler wsHandler, @NotNull Map<String, Object> attributes) {
        ResponseResult<Users> userResult = usersService.getCurrentUser();
        if (userResult == null || !userResult.isSuccess() || userResult.getData() == null) {
            response.setStatusCode(HttpStatus.UNAUTHORIZED);
            return false;
        }
        Users user = userResult.getData();
        attributes.put("userId", user.getUserId());
        attributes.put("account", user.getAccount());
        attributes.put("isAdmin", Boolean.TRUE.equals(user.getIsAdmin()));
        attributes.put("user", user);
        return true;
    }

    @Override
    public void afterHandshake(@NotNull ServerHttpRequest request, @NotNull ServerHttpResponse response, @NotNull WebSocketHandler wsHandler, Exception exception) {
    }
}
