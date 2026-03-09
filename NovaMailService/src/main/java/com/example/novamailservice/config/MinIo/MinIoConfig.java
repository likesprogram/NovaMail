package com.example.novamailservice.config.MinIo;

import io.minio.MinioClient;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class MinIoConfig {

    private final MinIoProperties minIoProperties;

    // 构造函数注入，Spring 自动注入 Bean
    public MinIoConfig(MinIoProperties minIoProperties) {
        this.minIoProperties = minIoProperties;
    }

    @Bean
    public MinioClient minioClient() {
        return MinioClient.builder()
                .endpoint(minIoProperties.getEndpoint())
                .credentials(minIoProperties.getUserName(), minIoProperties.getPassword())
                .build();
    }
}

