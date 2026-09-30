package com.example.novamailservice.config;

import com.example.novamailservice.config.Jwt.JwtInterceptor;
import jakarta.servlet.http.HttpServletRequest;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.util.AntPathMatcher;
import org.springframework.web.servlet.config.annotation.InterceptorRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

import java.util.List;

@Configuration
public class WebMvcConfig implements WebMvcConfigurer {
    private final JwtInterceptor jwtInterceptor;
    private static final List<String> EXCLUDE_PATHS = List.of(
            "/verification/**",
            "/authentication/register",
            "/authentication/login",
            "/authentication/refresh",
            "/authentication/revokeRefresh",
            "/authentication/resetPassword",
            "/userProfile/defaultAvatar",
            "/userProfile/avatar",
            "/userProfile/mailBoxAvatar",
            "/userFiles/shared/**",
            "/error"
    );
    private static final AntPathMatcher pathMatcher = new AntPathMatcher();

    public static boolean shouldHandle(HttpServletRequest request) {
        String uri = request.getRequestURI();
        for (String pattern : EXCLUDE_PATHS) {
            if (pathMatcher.match(pattern, uri)) return false;
        }
        Logger logger = LoggerFactory.getLogger(WebMvcConfig.class.getName());
        logger.info("{}", request.getRequestURI());
        return true;
    }

    @Autowired
    public WebMvcConfig(JwtInterceptor jwtInterceptor) {
        this.jwtInterceptor = jwtInterceptor;
    }

    @Override
    public void addInterceptors(InterceptorRegistry registry) {
        registry.addInterceptor(jwtInterceptor)
                .addPathPatterns("/**")
                .excludePathPatterns(EXCLUDE_PATHS);
    }
}
