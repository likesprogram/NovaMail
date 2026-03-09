package com.example.novamailservice.config.Jwt;

import com.example.novamailservice.config.Filter.RequestContext;
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
        if (status == JwtTokenProvider.TokenStatus.VALID) {
            return true;
        }

        response.setStatus(HttpServletResponse.SC_UNAUTHORIZED); // 401 未授权
        response.getWriter().write("Unauthorized: Invalid or missing token");
        return false;
    }
}