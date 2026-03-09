package com.example.novamailservice.config.Jwt;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;

@Data
@Configuration
@ConfigurationProperties(prefix = "jwt")
public class JwtProperties {
    // 密钥
    private String secret;
    // 密码加密力度
    private int strength;
    // 过期时间
    private int expiration;
}
