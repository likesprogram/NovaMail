package com.example.novamailservice.config.Jwt;

import com.example.novamailservice.config.Filter.RequestContext;
import com.example.novamailservice.dto.Result.AuthException;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.util.JwtTokenProvider;
import org.springframework.stereotype.Component;
import org.springframework.web.servlet.HandlerInterceptor;

import org.jetbrains.annotations.NotNull;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;

@Component
public class JwtInterceptor implements HandlerInterceptor {
    @Override
    public boolean preHandle(@NotNull HttpServletRequest request, @NotNull HttpServletResponse response, @NotNull Object handler) throws Exception {
        JwtTokenProvider.TokenStatus status = RequestContext.getCurrentTokenStatus();
        if (status == JwtTokenProvider.TokenStatus.VALID) return true;

        switch (status) {
            case EXPIRED -> throw new AuthException(ErrorCode.TOKEN_EXPIRED, "登录已过期");
            case ROBBED -> throw new AuthException(ErrorCode.TOKEN_ROBBED, "账号已在其他设备登录");
            case INVALID -> throw new AuthException(ErrorCode.TOKEN_INVALID, "Token 无效");
            default -> throw new AuthException(ErrorCode.UNAUTHORIZED, "未登录");
        }
    }
}