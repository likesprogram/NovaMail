package com.example.novamailservice.config.Jwt;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;

@Data
@Configuration
@ConfigurationProperties(prefix = "jwt")
public class JwtProperties {
    private String secret;
    private int strength;
    private int expiration;
    private int accessExpiration;
    private int refreshExpiration;

    public int getAccessExpirationSeconds() {
        return accessExpiration > 0 ? accessExpiration : expiration;
    }

    public int getRefreshExpirationSeconds() {
        int accessSeconds = getAccessExpirationSeconds();
        return refreshExpiration > 0 ? refreshExpiration : Math.max(accessSeconds * 24 * 7, accessSeconds);
    }
}
