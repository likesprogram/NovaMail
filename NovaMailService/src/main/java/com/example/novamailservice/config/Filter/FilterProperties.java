package com.example.novamailservice.config.Filter;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;

@Data
@Configuration
@ConfigurationProperties(prefix = "spring.security")
public class FilterProperties {
    private String[] allowedOrigins;
    private String[] allowedWhiteList;
}
