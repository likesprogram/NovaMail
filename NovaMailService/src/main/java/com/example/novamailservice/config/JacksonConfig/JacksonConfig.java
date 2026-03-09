package com.example.novamailservice.config.JacksonConfig;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializationFeature;
import com.fasterxml.jackson.databind.jsontype.impl.LaissezFaireSubTypeValidator;
import com.fasterxml.jackson.databind.module.SimpleModule;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Primary;

import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

@Configuration
public class JacksonConfig {
    private static final DateTimeFormatter FORMATTER = DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ss");

    @Bean
    public ObjectMapper objectMapper() {
        ObjectMapper mapper = new ObjectMapper();

        // 支持 Java 8 时间类型
        mapper.registerModule(new JavaTimeModule());
        mapper.disable(SerializationFeature.WRITE_DATES_AS_TIMESTAMPS);

        // 注册全局自定义的时间序列化器（如果你有的话）
        SimpleModule module = new SimpleModule();
        module.addSerializer(LocalDateTime.class, new GlobalLocalDateTimeSerializer(FORMATTER));
        module.addDeserializer(LocalDateTime.class, new GlobalLocalDateTimeDeserializer(FORMATTER));
        mapper.registerModule(module);

        return mapper;
    }
}
