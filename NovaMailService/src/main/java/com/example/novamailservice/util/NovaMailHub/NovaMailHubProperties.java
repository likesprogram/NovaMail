package com.example.novamailservice.util.NovaMailHub;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;


@Data
@Configuration
@ConfigurationProperties(prefix = "hub-server")
public class NovaMailHubProperties {
    private String address;
    private int port;
    private String userName;
    private String password;
}