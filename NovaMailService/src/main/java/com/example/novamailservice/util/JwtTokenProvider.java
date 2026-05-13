package com.example.novamailservice.util;


import com.auth0.jwt.JWT;
import com.auth0.jwt.algorithms.Algorithm;
import com.auth0.jwt.exceptions.TokenExpiredException;
import com.auth0.jwt.interfaces.DecodedJWT;
import com.example.novamailservice.config.Jwt.JwtProperties;
import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.Result.ResponseResult;
import lombok.Data;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.server.reactive.ServerHttpRequest;
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;
import org.springframework.stereotype.Component;

import jakarta.annotation.PostConstruct;
import jakarta.servlet.http.HttpServletRequest;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.Date;

// JWT Token 生成、验证、提取账号
@Component
public class JwtTokenProvider {
    private final Logger logger = LoggerFactory.getLogger(JwtTokenProvider.class.getName());
    private final JwtProperties jwtProperties;
    private final Cache cache;
    private BCryptPasswordEncoder passwordEncoder;
    private static final String TokenProviderCachePrefix = RedisConfig.NovaCache + "TokenProviderCache:";
    private static final String redisKeyPrefixToken = TokenProviderCachePrefix + "Token:";

    @Autowired
    public JwtTokenProvider(JwtProperties jwtProperties, Cache cache) {
        this.jwtProperties = jwtProperties;
        this.cache = cache;
    }

    @Data // 解析 Token 返回
    public static class ParseToken {
        TokenStatus tokenStatus; // 解析结果
        String account; // Token 中的账号
        String newToken; // Token 快过期时，会生成新的 Token
    }

    // Token 状态
    public enum TokenStatus {
        VALID, // 有效
        ROBBED, // 被抢
        EXPIRED, // 过期
        INVALID, // 无效
        VOID, // 空
    }

    @PostConstruct // 初始化配置
    public void init() {
        int strength = jwtProperties.getStrength();
        if (strength <= 0) throw new IllegalStateException("BCrypt strength 配置错误");
        passwordEncoder = new BCryptPasswordEncoder(strength);
    }

    // 生成 Token
    public String generateToken(String account) {
        // 账号空值验证
        if (account == null || account.isEmpty()) throw new IllegalArgumentException("account 不能为空");

        // secret 空值验证
        String secret = jwtProperties.getSecret();
        if (secret == null || secret.isEmpty()) throw new IllegalStateException("JWT secret 未配置");

        // 过期时间 验证
        long expireSeconds = jwtProperties.getExpiration();
        if (expireSeconds <= 0) throw new IllegalArgumentException("JWT 过期时间必须 > 0");

        // 时间计算
        Date now = new Date();
        Date expiration = new Date(System.currentTimeMillis() + expireSeconds * 1000);

        // 生成 Token 并缓存
        String token = JWT.create()
                .withSubject(account)
                .withIssuedAt(now)
                .withExpiresAt(expiration)
                .sign(Algorithm.HMAC256(jwtProperties.getSecret()));

        // 存 Redis（单端登录）
        cache.setCache(redisKeyPrefixToken + account, token, Duration.ofSeconds(expireSeconds + 120)); // 加 120 秒，防止延迟

        return token;
    }

    // 强制 Token 失效
    public void removeToken(String account) {
        // 账号空值验证
        if (account == null || account.isEmpty()) throw new IllegalArgumentException("account 不能为空");
        cache.removeCache(redisKeyPrefixToken + account);
    }

    // 验证 Token 是否有效, exchange：是否交换 Token，开启将会根据 Token 剩余时间 自动返回新 Token
    public ParseToken parseToken(String token, boolean exchange) {
        ParseToken parseToken = new ParseToken();
        parseToken.tokenStatus = TokenStatus.VOID;
        parseToken.account = null;
        parseToken.newToken = null;

        // 空值校验
        if (token == null || token.isEmpty()) return parseToken;

        try {
            // 验证 Token
            DecodedJWT jwt = JWT.require(Algorithm.HMAC256(jwtProperties.getSecret())).build().verify(token);

            // 从 token 获取 account
            String account = jwt.getSubject();

            // 从 Redis 获取当前 Token
            ResponseResult<String> cachedToken = cache.getCache(redisKeyPrefixToken + account, String.class, false);

            // 缓存查询校验，查不到可能是：管理员强行删除/过期
            if (!cachedToken.isSuccess()) {
                parseToken.tokenStatus = TokenStatus.INVALID;
                return parseToken;
            }

            // 缓存一致性验证，不一致可能是：其他客户端抢占
            if (!cachedToken.getData().equals(token)) {
                parseToken.tokenStatus = TokenStatus.ROBBED;
                return parseToken;
            }

            // Token 有效且正常
            parseToken.account = account;
            parseToken.tokenStatus = TokenStatus.VALID;

            if (exchange) {
                // 剩余时间计算
                long now = System.currentTimeMillis();
                long expireTime = jwt.getExpiresAt().getTime();
                long issuedAt = jwt.getIssuedAt().getTime();

                long remaining = expireTime - now;
                long total = expireTime - issuedAt;

                // 剩余时间小于 30% 时刷新
                if (remaining < total * 3 / 10) {
                    try { // 生成新的 Token
                        parseToken.newToken = generateToken(parseToken.account);
                        logger.info("账号[{}]的 Token 即将过期, 已经为其生成了新的 Token", parseToken.account);
                    } catch (IllegalArgumentException e) {
                        logger.error("Token生成失败[{}], account={}", e.getMessage(), account, e);
                    } catch (IllegalStateException e) {
                        logger.error("Token生成失败[系统配置错误], account={}", account, e);
                    } catch (Exception e) {
                        logger.error("Token生成失败 account={}", account, e);
                    }
                }
            }
        } catch (TokenExpiredException e) {
            parseToken.tokenStatus = TokenStatus.EXPIRED;
        } catch (Exception e) {
            parseToken.tokenStatus = TokenStatus.INVALID;
        }
        return parseToken;
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

    // 匹配/验证 密码
    public boolean matchesPassword(String rawPassword, String encodedPassword) {
        try {
            if (encodedPassword == null || encodedPassword.trim().isEmpty()) return false;
            return passwordEncoder.matches(rawPassword, encodedPassword);
        } catch (IllegalArgumentException e) {
            return false;
        }
    }

    // 编码/加密 密码
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
