package com.example.novamailservice.config.Filter;

import com.example.novamailservice.config.WebMvcConfig;
import com.example.novamailservice.dto.DeviceInfo;
import com.example.novamailservice.dto.IpInfo;
import com.example.novamailservice.entity.Users.UserLoginLog;
import com.example.novamailservice.util.DeviceInfoParser;
import com.example.novamailservice.util.JwtTokenProvider;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.jetbrains.annotations.NotNull;

import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;

public class RequestContext {
    private static final ThreadLocal<Map<String, Object>> contextHolder = ThreadLocal.withInitial(HashMap::new);

    private static final String IP_KEY = "ip";
    private static final String ACCOUNT_KEY = "account";
    private static final String ACCESS_TOKEN_KEY = "accessToken";
    private static final String TOKEN_STATUS_KEY = "tokenStatus";
    private static final String DEVICE_KEY = "device";
    private static final String LOGIN_LOG = "loginLog";

    public static void setCurrentIp(IpInfo ip) {
        getContext().put(IP_KEY, ip);
    }

    public static IpInfo getCurrentIp() {
        return (IpInfo) getContext().get(IP_KEY);
    }

    public static JwtTokenProvider.TokenStatus getCurrentTokenStatus() {
        return (JwtTokenProvider.TokenStatus) getContext().get(TOKEN_STATUS_KEY);
    }

    public static void setCurrentAccount(String account) {
        getContext().put(ACCOUNT_KEY, account);
    }

    public static void setCurrentAccount(@NotNull HttpServletRequest request, HttpServletResponse response, @NotNull JwtTokenProvider jwtTokenProvider) {
        String token = JwtTokenProvider.extractTokenFromRequest(request);
        JwtTokenProvider.ParseToken parseToken = jwtTokenProvider.parseToken(token, WebMvcConfig.shouldHandle(request));
        if (parseToken.getTokenStatus() == JwtTokenProvider.TokenStatus.VALID) {
            setCurrentAccount(parseToken.getAccount());
            setCurrentAccessToken(token);
            if (parseToken.getNewToken() != null && !parseToken.getNewToken().isEmpty()) {
                response.setHeader("X-New-Token", parseToken.getNewToken());
            }
        }
        setCurrentTokenStatus(parseToken.getTokenStatus());
    }

    public static void setLoginLog(@NotNull UserLoginLog userLoginLog) {
        getContext().put(LOGIN_LOG, userLoginLog);
    }

    public static UserLoginLog getLoginLog() {
        return (UserLoginLog) getContext().get(LOGIN_LOG);
    }

    public static String getCurrentAccount() {
        return (String) getContext().get(ACCOUNT_KEY);
    }

    public static String getCurrentAccessToken() {
        return (String) getContext().get(ACCESS_TOKEN_KEY);
    }

    public static void setDeviceInfo(HttpServletRequest request) {
        String userAgent = request.getHeader("User-Agent");
        Map<String, String> headers = new HashMap<>();
        Enumeration<String> headerNames = request.getHeaderNames();
        while (headerNames.hasMoreElements()) {
            String headerName = headerNames.nextElement();
            headers.put(headerName, request.getHeader(headerName));
        }

        DeviceInfo device = DeviceInfoParser.parse(userAgent, headers);
        getContext().put(DEVICE_KEY, device);
    }

    public static DeviceInfo getDevice() {
        return (DeviceInfo) getContext().get(DEVICE_KEY);
    }

    public static void clear() {
        contextHolder.remove();
    }

    private static void setCurrentTokenStatus(JwtTokenProvider.TokenStatus status) {
        getContext().put(TOKEN_STATUS_KEY, status);
    }

    private static void setCurrentAccessToken(String token) {
        getContext().put(ACCESS_TOKEN_KEY, token);
    }

    private static Map<String, Object> getContext() {
        return contextHolder.get();
    }
}
