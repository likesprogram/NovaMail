package com.example.novamailservice.config.Filter;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;

@Data
@Configuration
@ConfigurationProperties(prefix = "ip-region")
public class IpRegionProperties {
    private String updateUrl;
    private String localPath;
    private String updateCron;
}
