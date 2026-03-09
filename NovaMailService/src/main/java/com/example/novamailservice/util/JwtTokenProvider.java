package com.example.novamailservice.util;


import com.auth0.jwt.JWT;
import com.auth0.jwt.JWTVerifier;
import com.auth0.jwt.algorithms.Algorithm;
import com.auth0.jwt.interfaces.DecodedJWT;
import com.example.novamailservice.config.Jwt.JwtProperties;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.server.reactive.ServerHttpRequest;
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;
import org.springframework.stereotype.Component;

import jakarta.annotation.PostConstruct;
import jakarta.servlet.http.HttpServletRequest;
import java.nio.charset.StandardCharsets;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;

// JWT Token 生成、验证、提取账号
@Component
public class JwtTokenProvider {
    private final JwtProperties jwtProperties;
    private BCryptPasswordEncoder passwordEncoder;

    @Autowired
    public JwtTokenProvider(JwtProperties jwtProperties) {
        this.jwtProperties = jwtProperties;
    }

    public enum TokenStatus {
        VALID, // 有效
        EXPIRED, // 过期
        INVALID, // 无效
        VOID, // 空
    }

    @PostConstruct
    public void init() {
        passwordEncoder = new BCryptPasswordEncoder(jwtProperties.getStrength());
    }

    // 生成 Token
    public String generateToken(String account) {
        HashMap<String, Object> map = new HashMap<>();

        Calendar instance = Calendar.getInstance();
        instance.add(Calendar.SECOND, jwtProperties.getExpiration()); // 确保单位正确

        return JWT.create()
                .withHeader(map)
                .withClaim("Account", account)
                .withExpiresAt(instance.getTime())
                .sign(Algorithm.HMAC256(jwtProperties.getSecret()));
    }

    // 从 Token 获取账号
    public String getAccountFromToken(String token) {
        if (validateToken(token) == TokenStatus.VALID)
            return JWT.require(Algorithm.HMAC256(jwtProperties.getSecret()))
                    .build()
                    .verify(token)
                    .getClaim("Account")
                    .asString();
        return null;
    }

    // 验证 Token 是否有效
    public TokenStatus validateToken(String token) {
        if (token == null || token.isEmpty()) return TokenStatus.VOID;
        try {
            JWTVerifier verifier = JWT.require(Algorithm.HMAC256(jwtProperties.getSecret())).build();
            DecodedJWT jwt = verifier.verify(token);
            return jwt.getExpiresAt().before(new Date()) ? TokenStatus.EXPIRED : TokenStatus.VALID;
        } catch (Exception e) {
            return TokenStatus.INVALID;
        }
    }

    // 提取 Token 非阻塞模式
    public static String extractTokenFromRequest(ServerHttpRequest request) {
        // 从请求头中获取
        String bearerToken = request.getHeaders().getFirst("Authorization");
        if (bearerToken != null && bearerToken.startsWith("Bearer ")) {
            return bearerToken.substring(7);
        }
        // 从参数中获取
        String tokenFromParam = request.getQueryParams().getFirst("token");
        if (tokenFromParam != null && !tokenFromParam.isEmpty()) {
            return tokenFromParam;
        }
        return null;
    }

    // 提取 Token 阻塞模式
    public static String extractTokenFromRequest(HttpServletRequest request) {
        // 从请求头中获取
        String bearerToken = request.getHeader("Authorization");
        if (bearerToken != null && bearerToken.startsWith("Bearer ")) {
            return bearerToken.substring(7);
        }
        // 从参数中获取
        String tokenFromParam = request.getParameter("token");
        if (tokenFromParam != null && !tokenFromParam.isEmpty()) {
            return tokenFromParam;
        }
        return null;
    }

    public boolean matchesPassword(String rawPassword, String encodedPassword) {
        try {
            if (encodedPassword == null || encodedPassword.trim().isEmpty()) return false;
            return passwordEncoder.matches(rawPassword, encodedPassword);
        } catch (IllegalArgumentException e) {
            return false;
        }
    }

    public String encodePassword(String rawPassword) {
        if (rawPassword == null || rawPassword.isEmpty()) {
            throw new IllegalArgumentException("Raw password cannot be empty");
        }
        if (rawPassword.getBytes(StandardCharsets.UTF_8).length > 72) {
            throw new IllegalArgumentException("Password exceeds 72 bytes length limit");
        }
        return passwordEncoder.encode(rawPassword);
    }
}
