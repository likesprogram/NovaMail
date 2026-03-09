package com.example.novamailservice.config.Filter;

import com.example.novamailservice.dto.DeviceInfo;
import com.example.novamailservice.dto.IpInfo;
import com.example.novamailservice.util.DeviceInfoParser;
import com.example.novamailservice.util.JwtTokenProvider;
import org.jetbrains.annotations.NotNull;
import jakarta.servlet.http.HttpServletRequest;
import java.util.*;

public class RequestContext {
    // 使用ThreadLocal存储请求上下文信息
    private static final ThreadLocal<Map<String, Object>> contextHolder =
            ThreadLocal.withInitial(HashMap::new);

    // 常量键名
    private static final String IP_KEY = "ip";
    private static final String ACCOUNT_KEY = "account";
    private static final String TOKEN_STATUS_KEY = "tokenStatus";
    private static final String DEVICE_KEY = "device";
    private static final String USER_AGENT_KEY = "userAgent";

    /**
     * 设置客户端IP
     */
    public static void setCurrentIp(IpInfo ip) {
        getContext().put(IP_KEY, ip);
    }

    /**
     * 获取客户端IP
     */
    public static IpInfo getCurrentIp() {
        return (IpInfo) getContext().get(IP_KEY);
    }

    /**
     * 获取当前token状态
     */
    public static JwtTokenProvider.TokenStatus getCurrentTokenStatus() {
        return (JwtTokenProvider.TokenStatus) getContext().get(TOKEN_STATUS_KEY);
    }

    /**
     * 设置当前账号
     */
    public static void setCurrentAccount(String account) {
        getContext().put(ACCOUNT_KEY, account);
    }
    public static void setCurrentAccount(@NotNull HttpServletRequest request, @NotNull JwtTokenProvider jwtTokenProvider) {
        String token = JwtTokenProvider.extractTokenFromRequest(request);
        JwtTokenProvider.TokenStatus status = jwtTokenProvider.validateToken(token);
        if (status == JwtTokenProvider.TokenStatus.VALID) {
            // 存入 RequestContext，整个请求周期可用
            setCurrentAccount(jwtTokenProvider.getAccountFromToken(token));
        }
        setCurrentTokenStatus(status);
    }

    /**
     * 获取当前账号
     */
    public static String getCurrentAccount() {
        return (String) getContext().get(ACCOUNT_KEY);
    }

    /**
     * 设置设备信息（完整User-Agent字符串）
     */
    public static void setDeviceInfo(HttpServletRequest request) {
        String userAgent = request.getHeader("User-Agent");
        getContext().put(USER_AGENT_KEY, userAgent);

        // 将 HttpServletRequest 的 Headers 转为 Map<String, String>
        Map<String, String> headers = new HashMap<>();
        Enumeration<String> headerNames = request.getHeaderNames();
        while (headerNames.hasMoreElements()) {
            String headerName = headerNames.nextElement();
            headers.put(headerName, request.getHeader(headerName));
        }

        // 解析设备类型
        DeviceInfo device = DeviceInfoParser.parse(userAgent, headers);

        getContext().put(DEVICE_KEY, device);
    }

    /**
     * 获取原始User-Agent字符串
     */
    public static String getUserAgent() {
        return (String) getContext().get(USER_AGENT_KEY);
    }

    /**
     * 获取解析后的设备信息
     */
    public static DeviceInfo getDevice() {
        return (DeviceInfo) getContext().get(DEVICE_KEY);
    }

    /**
     * 清除上下文
     */
    public static void clear() {
        contextHolder.remove();
    }

    // 私有方法设置当前token状态
    private static void setCurrentTokenStatus(JwtTokenProvider.TokenStatus status) {
        getContext().put(TOKEN_STATUS_KEY, status);
    }

    // 私有方法获取上下文Map
    private static Map<String, Object> getContext() {
        return contextHolder.get();
    }
}
