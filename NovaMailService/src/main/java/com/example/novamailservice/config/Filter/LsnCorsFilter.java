package com.example.novamailservice.config.Filter;

import java.io.IOException;
import java.net.URI;
import java.util.Set;

import jakarta.servlet.annotation.WebFilter;

import com.example.novamailservice.dto.IpInfo;
import com.example.novamailservice.util.IpRegion;
import com.example.novamailservice.util.JwtTokenProvider;
import org.springframework.beans.factory.annotation.Autowired;

import jakarta.servlet.Filter;
import jakarta.servlet.FilterChain;
import jakarta.servlet.FilterConfig;
import jakarta.servlet.ServletException;
import jakarta.servlet.ServletRequest;
import jakarta.servlet.ServletResponse;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import lombok.extern.slf4j.Slf4j;

/** 动态配置跨域，允许外部请求进行增加, asyncSupported 异步请求 */
@WebFilter(urlPatterns = "/*", asyncSupported = true)
@Slf4j
public class LsnCorsFilter implements Filter {
    private final FilterProperties filterProperties;
    private final AllowedOriginsBean corsOrigins;
    private final JwtTokenProvider jwtTokenProvider;
    private final IpRegion ipRegion;

    @Autowired
    public LsnCorsFilter(FilterProperties filterProperties, AllowedOriginsBean corsOrigins, JwtTokenProvider jwtTokenProvider, IpRegion ipRegion) {
        this.filterProperties = filterProperties;
        this.corsOrigins = corsOrigins;
        this.jwtTokenProvider = jwtTokenProvider;
        this.ipRegion = ipRegion;
    }

    @Override
    public void init(FilterConfig filterConfig) {
        // 初始化过滤器
        if (filterProperties.getAllowedOrigins() != null) {
            corsOrigins.addInBatch(filterProperties.getAllowedOrigins());
        }
    }

    private boolean isInList(String origin) {
        if (corsOrigins != null) {
            Set<String> oriList = corsOrigins.getOrigin();
            if (oriList != null) {
                try {
                    URI uri = new URI(origin);
                    String host = uri.getHost(); // 提取域名或IP
                    return oriList.contains(host);
                } catch (Exception e) {
                    log.warn("Origin 格式错误：{}", origin);
                }
            }
        }
        return false;
    }

    @Override
    public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain)
            throws IOException, ServletException {
        RequestContext.clear(); // 清除线程中的数据

        HttpServletResponse res = (HttpServletResponse) response;
        HttpServletRequest req = (HttpServletRequest) request;

        // 获取IP地址
        IpInfo ipInfo = new IpInfo();
        ipInfo.setIpAddress(getClientIp(req));

        // 获取请求的源（Origin）
        String origin = req.getHeader("Origin");
        String originPrefix = req.getScheme() + "://" + req.getRemoteHost();
        boolean isAllowed;
        if (origin == null) {
            origin = originPrefix + ":" + req.getRemotePort();
        }
        isAllowed = isInList(origin);
        if (!isAllowed) {
            isAllowed = isInList(originPrefix);
            if (!isAllowed) {
                log.warn("来自【{}】的请求未在允许列表中，跨域访问被拦截", origin);
                res.setStatus(HttpServletResponse.SC_FORBIDDEN);
                return;
            }
        }

        // 设置CORS头
        res.setHeader("Access-Control-Allow-Origin", origin);
        // res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE,
        // OPTIONS");
        res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS,DELETE");
        // res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization,
        // X-Requested-With");
        res.setHeader("Access-Control-Allow-Headers", "*");
        res.setHeader("Access-Control-Allow-Credentials", "true");
        res.setHeader("access-control-max-age", "3600");

        // 对于预检请求（OPTIONS），直接返回响应，不继续链中的其他过滤器或目标资源
        if ("OPTIONS".equalsIgnoreCase(req.getMethod())) {
            res.setStatus(HttpServletResponse.SC_OK);
            return;
        }

        // 获取IP归属地
        ipInfo.setIpRegion(ipRegion.getRegion(ipInfo.getIpAddress()));

        // 获取客户端真实IP
        RequestContext.setCurrentIp(ipInfo);

        // 获取账号 (此项目放到了 WebMvcConfigurer 中获取)
//        RequestContext.setCurrentAccount(req.getHeader("account"));
        RequestContext.setCurrentAccount(req, jwtTokenProvider);

        // 获取设备信息
        RequestContext.setDeviceInfo(req);

        log.info("接收到来自【{} : {}】的请求", origin, ipInfo.getIpRegion());

        // 继续链中的其他过滤器或目标资源
        chain.doFilter(request, response);
    }

    /**
     * 获取客户端真实IP
     */
    private String getClientIp(HttpServletRequest request) {
        String ip = request.getHeader("X-Forwarded-For");

        // 如果有多个 IP 地址，以逗号分隔，取第一个
        if (ip != null && !ip.isEmpty() && !"unknown".equalsIgnoreCase(ip)) {
            String[] ips = ip.split(",");
            return ips[0].trim();  // 取第一个 IP
        }

        // 如果没有 X-Forwarded-For 或其值无效，回退到 Proxy-Client-IP
        ip = request.getHeader("Proxy-Client-IP");
        if (ip != null && !ip.isEmpty() && !"unknown".equalsIgnoreCase(ip)) {
            return ip;
        }

        // 如果没有 Proxy-Client-IP，回退到 WL-Proxy-Client-IP
        ip = request.getHeader("WL-Proxy-Client-IP");
        if (ip != null && !ip.isEmpty() && !"unknown".equalsIgnoreCase(ip)) {
            return ip;
        }

        // 如果都没有，再回退到 getRemoteAddr()
        ip = request.getRemoteAddr();
        return ip;
    }

    @Override
    public void destroy() {
        // 销毁过滤器（如果需要）
        RequestContext.clear();
    }
}
