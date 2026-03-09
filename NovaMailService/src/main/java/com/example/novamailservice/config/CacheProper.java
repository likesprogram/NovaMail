package com.example.novamailservice.config;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;

import java.time.Duration;

@Data
@Configuration
@ConfigurationProperties(prefix = "cache")
public class CacheProper {
    // 每次命中增加缓存时长
    private Duration extend = Duration.ofMinutes(30);
    // 缓存最大时长
    private Duration max = Duration.ofDays(1);
    // 缓存时长
    private Duration duration = Duration.ofMinutes(30);
}
