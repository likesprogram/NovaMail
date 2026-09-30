package com.example.novamailservice.util;

import com.auth0.jwt.JWT;
import com.auth0.jwt.algorithms.Algorithm;
import com.auth0.jwt.exceptions.TokenExpiredException;
import com.auth0.jwt.interfaces.DecodedJWT;
import com.example.novamailservice.config.Jwt.JwtProperties;
import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.LoginToken;
import jakarta.annotation.PostConstruct;
import jakarta.servlet.http.HttpServletRequest;
import lombok.Data;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.server.reactive.ServerHttpRequest;
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;
import org.springframework.stereotype.Component;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.time.Duration;
import java.util.Date;
import java.util.HexFormat;
import java.util.UUID;

@Component
public class JwtTokenProvider {
    private final JwtProperties jwtProperties;
    private final Cache cache;
    private BCryptPasswordEncoder passwordEncoder;

    private static final String TOKEN_TYPE_CLAIM = "type";
    private static final String TOKEN_VERSION_CLAIM = "version";
    private static final String ACCESS_TOKEN_TYPE = "access";
    private static final String REFRESH_TOKEN_TYPE = "refresh";
    private static final String TOKEN_PROVIDER_CACHE_PREFIX = RedisConfig.NovaCache + "TokenProviderCache:";
    private static final String REDIS_KEY_PREFIX_REFRESH_TOKEN = TOKEN_PROVIDER_CACHE_PREFIX + "RefreshToken:";
    private static final String REDIS_KEY_PREFIX_INVALID_ACCESS_TOKEN = TOKEN_PROVIDER_CACHE_PREFIX + "InvalidAccessToken:";
    private static final String REDIS_KEY_PREFIX_INVALID_REFRESH_TOKEN = TOKEN_PROVIDER_CACHE_PREFIX + "InvalidRefreshToken:";
    private static final String REDIS_KEY_PREFIX_ACCOUNT_TOKEN_VERSION = TOKEN_PROVIDER_CACHE_PREFIX + "AccountTokenVersion:";
    private static final long CACHE_DELAY_SECONDS = 120L;

    @Autowired
    public JwtTokenProvider(JwtProperties jwtProperties, Cache cache) {
        this.jwtProperties = jwtProperties;
        this.cache = cache;
    }

    @Data
    public static class ParseToken {
        TokenStatus tokenStatus;
        String account;
        String newToken;
    }

    public enum TokenStatus {
        VALID,
        ROBBED,
        EXPIRED,
        INVALID,
        VOID,
    }

    @PostConstruct
    public void init() {
        int strength = jwtProperties.getStrength();
        if (strength <= 0) throw new IllegalStateException("BCrypt strength 配置错误");
        passwordEncoder = new BCryptPasswordEncoder(strength);
    }

    public LoginToken generateTokenPair(String account) {
        long accessExpiration = getAccessExpirationSeconds();
        long refreshExpiration = getRefreshExpirationSeconds();
        long tokenVersion = getAccountTokenVersion(account);
        String accessToken = generateToken(account, ACCESS_TOKEN_TYPE, accessExpiration, tokenVersion);
        String refreshToken = generateToken(account, REFRESH_TOKEN_TYPE, refreshExpiration, tokenVersion);

        cache.setCache(refreshTokenKey(account), refreshToken, Duration.ofSeconds(refreshExpiration + CACHE_DELAY_SECONDS));
        return new LoginToken(accessToken, refreshToken, accessExpiration, refreshExpiration);
    }

    public String generateToken(String account) {
        return generateToken(account, ACCESS_TOKEN_TYPE, getAccessExpirationSeconds(), getAccountTokenVersion(account));
    }

    public ResponseResult<LoginToken> refreshToken(String refreshToken) {
        ParseToken parsedToken = parseRefreshToken(refreshToken);
        return switch (parsedToken.getTokenStatus()) {
            case VALID -> {
                revokeRefreshToken(refreshToken);
                yield ResponseResult.success(generateTokenPair(parsedToken.getAccount()));
            }
            case EXPIRED -> ResponseResult.error(ErrorCode.TOKEN_EXPIRED, "长 Token 已过期");
            case VOID -> ResponseResult.error(ErrorCode.UNAUTHORIZED, "未提供长 Token");
            default -> ResponseResult.error(ErrorCode.TOKEN_INVALID, "长 Token 无效");
        };
    }

    public void removeToken(String account) {
        revokeAccountTokens(account);
    }

    public void revokeAccountTokens(String account) {
        if (account == null || account.isBlank()) return;
        cache.setCache(accountTokenVersionKey(account), System.currentTimeMillis(), Duration.ofSeconds(getRefreshExpirationSeconds() + CACHE_DELAY_SECONDS));

        ResponseResult<String> cachedRefreshToken = cache.getCache(refreshTokenKey(account), String.class, false);
        if (cachedRefreshToken.isSuccess()) revokeRefreshToken(cachedRefreshToken.getData());
        cache.removeCache(refreshTokenKey(account));
    }

    public void revokeCurrentRefreshToken(String account) {
        if (account == null || account.isBlank()) return;

        ResponseResult<String> cachedRefreshToken = cache.getCache(refreshTokenKey(account), String.class, false);
        if (cachedRefreshToken.isSuccess()) revokeRefreshToken(cachedRefreshToken.getData());
        cache.removeCache(refreshTokenKey(account));
    }

    public void revokeAccessToken(String accessToken) {
        revokeToken(accessToken, ACCESS_TOKEN_TYPE, REDIS_KEY_PREFIX_INVALID_ACCESS_TOKEN);
    }

    public void revokeRefreshToken(String refreshToken) {
        DecodedJWT jwt = revokeToken(refreshToken, REFRESH_TOKEN_TYPE, REDIS_KEY_PREFIX_INVALID_REFRESH_TOKEN);
        if (jwt == null) return;

        String account = jwt.getSubject();
        if (account == null || account.isBlank()) return;

        ResponseResult<String> cachedRefreshToken = cache.getCache(refreshTokenKey(account), String.class, false);
        if (cachedRefreshToken.isSuccess() && refreshToken.equals(cachedRefreshToken.getData())) {
            cache.removeCache(refreshTokenKey(account));
        }
    }

    public ParseToken parseToken(String token, boolean exchange) {
        ParseToken parseToken = emptyParseToken();
        if (token == null || token.isBlank()) return parseToken;

        try {
            DecodedJWT jwt = verifyToken(token);
            String account = jwt.getSubject();

            if (account == null || account.isBlank() || !ACCESS_TOKEN_TYPE.equals(jwt.getClaim(TOKEN_TYPE_CLAIM).asString())) {
                parseToken.tokenStatus = TokenStatus.INVALID;
                return parseToken;
            }

            if (isTokenInvalid(token, REDIS_KEY_PREFIX_INVALID_ACCESS_TOKEN) || isAccountTokenVersionInvalid(account, jwt)) {
                parseToken.tokenStatus = TokenStatus.INVALID;
                return parseToken;
            }

            parseToken.account = account;
            parseToken.tokenStatus = TokenStatus.VALID;

            if (exchange && shouldExchange(jwt)) {
                parseToken.newToken = generateToken(account);
            }
        } catch (TokenExpiredException e) {
            parseToken.tokenStatus = TokenStatus.EXPIRED;
        } catch (Exception e) {
            parseToken.tokenStatus = TokenStatus.INVALID;
        }

        return parseToken;
    }

    private ParseToken parseRefreshToken(String token) {
        ParseToken parseToken = emptyParseToken();
        if (token == null || token.isBlank()) return parseToken;

        try {
            DecodedJWT jwt = verifyToken(token);
            String account = jwt.getSubject();

            if (account == null || account.isBlank() || !REFRESH_TOKEN_TYPE.equals(jwt.getClaim(TOKEN_TYPE_CLAIM).asString())) {
                parseToken.tokenStatus = TokenStatus.INVALID;
                return parseToken;
            }

            ResponseResult<String> cachedRefreshToken = cache.getCache(refreshTokenKey(account), String.class, false);
            if (!cachedRefreshToken.isSuccess()
                    || !token.equals(cachedRefreshToken.getData())
                    || isTokenInvalid(token, REDIS_KEY_PREFIX_INVALID_REFRESH_TOKEN)
                    || isAccountTokenVersionInvalid(account, jwt)) {
                parseToken.tokenStatus = TokenStatus.INVALID;
                return parseToken;
            }

            parseToken.account = account;
            parseToken.tokenStatus = TokenStatus.VALID;
        } catch (TokenExpiredException e) {
            parseToken.tokenStatus = TokenStatus.EXPIRED;
        } catch (Exception e) {
            parseToken.tokenStatus = TokenStatus.INVALID;
        }

        return parseToken;
    }

    private ParseToken emptyParseToken() {
        ParseToken parseToken = new ParseToken();
        parseToken.tokenStatus = TokenStatus.VOID;
        parseToken.account = null;
        parseToken.newToken = null;
        return parseToken;
    }

    private String generateToken(String account, String tokenType, long expireSeconds, long tokenVersion) {
        if (account == null || account.isBlank()) throw new IllegalArgumentException("account 不能为空");

        String secret = jwtProperties.getSecret();
        if (secret == null || secret.isBlank()) throw new IllegalStateException("JWT secret 未配置");
        if (expireSeconds <= 0) throw new IllegalArgumentException("JWT 过期时间必须 > 0");

        Date now = new Date();
        Date expiration = new Date(System.currentTimeMillis() + expireSeconds * 1000);

        return JWT.create()
                .withSubject(account)
                .withJWTId(UUID.randomUUID().toString())
                .withClaim(TOKEN_TYPE_CLAIM, tokenType)
                .withClaim(TOKEN_VERSION_CLAIM, tokenVersion)
                .withIssuedAt(now)
                .withExpiresAt(expiration)
                .sign(Algorithm.HMAC256(secret));
    }

    private DecodedJWT verifyToken(String token) {
        return JWT.require(Algorithm.HMAC256(jwtProperties.getSecret())).build().verify(token);
    }

    private DecodedJWT revokeToken(String token, String expectedTokenType, String redisKeyPrefix) {
        if (token == null || token.isBlank()) return null;

        try {
            DecodedJWT jwt = verifyToken(token);
            if (!expectedTokenType.equals(jwt.getClaim(TOKEN_TYPE_CLAIM).asString())) return null;

            long remainingSeconds = (jwt.getExpiresAt().getTime() - System.currentTimeMillis()) / 1000;
            if (remainingSeconds <= 0) return null;

            cache.setCache(redisKeyPrefix + sha256Hex(token), true, Duration.ofSeconds(remainingSeconds + CACHE_DELAY_SECONDS));
            return jwt;
        } catch (Exception e) {
            return null;
        }
    }

    private boolean isTokenInvalid(String token, String redisKeyPrefix) {
        return cache.getCache(redisKeyPrefix + sha256Hex(token), Boolean.class, false).isSuccess();
    }

    private boolean isAccountTokenVersionInvalid(String account, DecodedJWT jwt) {
        Long tokenVersion = jwt.getClaim(TOKEN_VERSION_CLAIM).asLong();
        return tokenVersion == null || !tokenVersion.equals(getAccountTokenVersion(account));
    }

    private boolean shouldExchange(DecodedJWT jwt) {
        long now = System.currentTimeMillis();
        long expireTime = jwt.getExpiresAt().getTime();
        long issuedAt = jwt.getIssuedAt().getTime();
        long remaining = expireTime - now;
        long total = expireTime - issuedAt;
        return remaining > 0 && total > 0 && remaining < total * 3 / 10;
    }

    private String refreshTokenKey(String account) {
        return REDIS_KEY_PREFIX_REFRESH_TOKEN + account;
    }

    private long getAccountTokenVersion(String account) {
        ResponseResult<Long> tokenVersion = cache.getCache(accountTokenVersionKey(account), Long.class, false);
        return tokenVersion.isSuccess() ? tokenVersion.getData() : 0L;
    }

    private String accountTokenVersionKey(String account) {
        return REDIS_KEY_PREFIX_ACCOUNT_TOKEN_VERSION + account;
    }

    private long getAccessExpirationSeconds() {
        return jwtProperties.getAccessExpirationSeconds();
    }

    private long getRefreshExpirationSeconds() {
        return jwtProperties.getRefreshExpirationSeconds();
    }

    private String sha256Hex(String value) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            return HexFormat.of().formatHex(digest.digest(value.getBytes(StandardCharsets.UTF_8)));
        } catch (Exception e) {
            throw new IllegalStateException("SHA-256 unavailable", e);
        }
    }

    public static String extractTokenFromRequest(ServerHttpRequest request) {
        String bearerToken = request.getHeaders().getFirst("Authorization");
        if (bearerToken != null && bearerToken.startsWith("Bearer ")) return bearerToken.substring(7);

        String tokenFromParam = request.getQueryParams().getFirst("token");
        if (tokenFromParam != null && !tokenFromParam.isEmpty()) return tokenFromParam;

        return null;
    }

    public static String extractTokenFromRequest(HttpServletRequest request) {
        String bearerToken = request.getHeader("Authorization");
        if (bearerToken != null && bearerToken.startsWith("Bearer ")) return bearerToken.substring(7);

        String tokenFromParam = request.getParameter("token");
        if (tokenFromParam != null && !tokenFromParam.isEmpty()) return tokenFromParam;

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
        if (rawPassword == null || rawPassword.isEmpty()) throw new IllegalArgumentException("Raw password cannot be empty");
        if (rawPassword.getBytes(StandardCharsets.UTF_8).length > 72) {
            throw new IllegalArgumentException("Password exceeds 72 bytes length limit");
        }
        return passwordEncoder.encode(rawPassword);
    }
}
