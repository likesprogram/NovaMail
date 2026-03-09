package com.example.novamailservice.config.MinIo;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;

@Data
@Configuration
@ConfigurationProperties(prefix = "minio")
public class MinIoProperties {
    // minio地址+端口号
    private String endpoint;

    // minio用户名
    private String userName;

    // minio密码
    private String password;
}
