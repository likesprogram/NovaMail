package com.example.novamailservice.config;

import com.fasterxml.jackson.databind.ObjectMapper;
import io.lettuce.core.api.StatefulRedisConnection;
import io.lettuce.core.api.sync.RedisCommands;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.data.redis.connection.RedisConnection;
import org.springframework.data.redis.connection.RedisConnectionFactory;
import org.springframework.data.redis.connection.jedis.JedisConnectionFactory;
import org.springframework.data.redis.connection.lettuce.LettuceConnectionFactory;
import org.springframework.data.redis.core.RedisTemplate;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.data.redis.serializer.Jackson2JsonRedisSerializer;
import org.springframework.data.redis.serializer.StringRedisSerializer;

@Configuration
public class RedisConfig {
    private final Logger logger = LoggerFactory.getLogger(RedisConfig.class.getName());
    private final RedisConnectionFactory factory;
    @Autowired
    public RedisConfig(RedisConnectionFactory redisProperties) {
        this.factory = redisProperties;
        checkRedisClient();
    }
    private void checkRedisClient() {
        try {
            String serverVersion = "未知";
            String redisMode = "未知";
            try (RedisConnection connection = factory.getConnection()) {
                var serverInfo = connection.info("server");
                if (serverInfo != null) {
                    serverVersion = serverInfo.getProperty("redis_version", "未知");
                }
                var replicationInfo = connection.info("replication");
                if (replicationInfo != null) {
                    redisMode = replicationInfo.getProperty("role", "未知");
                }
            } catch (Exception e) {
                logger.warn("无法获取 Redis 服务器信息: {}", e.getMessage());
            }

            logger.info("🧩 Redis 服务器版本: {}", serverVersion);

            logger.info("🔗 连接模式: {}", redisMode);
            if (factory instanceof LettuceConnectionFactory lettuceFactory) {

                String lettuceVersion = io.lettuce.core.RedisClient.class
                        .getPackage()
                        .getImplementationVersion();

                var clientConfig = lettuceFactory.getClientConfiguration();
                var redisConfig = lettuceFactory.getStandaloneConfiguration();
                var database = lettuceFactory.getDatabase();

                logger.info("✅ 当前 Redis 客户端: Lettuce");
                logger.info("🌿 Lettuce 客户端版本: {}", lettuceVersion != null ? lettuceVersion : "未知");
                logger.info("🌐 服务器地址: {}:{}", redisConfig.getHostName(), redisConfig.getPort());
                logger.info("🧱 使用数据库: {}", redisConfig.getDatabase());
                logger.info("⚙️客户端配置类型: {}", clientConfig.getClass().getSimpleName());
            } else {
                logger.warn("⚠ 未检测到 LettuceConnectionFactory，当前使用的是: {}",
                        factory.getClass().getName());
            }
        } catch (Exception e) {
            logger.error("检查 Redis 客户端和版本信息时出错: {}", e.getMessage(), e);
        }
    }

    @Bean
    public RedisTemplate<String, Object> redisTemplate(ObjectMapper objectMapper) {
        RedisTemplate<String, Object> template = new RedisTemplate<>();
        template.setConnectionFactory(factory);

        // 使用全局的 ObjectMapper
        Jackson2JsonRedisSerializer<Object> serializer = new Jackson2JsonRedisSerializer<>(objectMapper, Object.class);

        template.setKeySerializer(new StringRedisSerializer());
        template.setValueSerializer(serializer);
        template.setHashKeySerializer(new StringRedisSerializer());
        template.setHashValueSerializer(serializer);

        template.afterPropertiesSet();
        return template;
    }

    @Bean
    public StringRedisTemplate stringRedisTemplate() {
        StringRedisTemplate template = new StringRedisTemplate();
        template.setConnectionFactory(factory);
        return template;
    }

    public static String NovaCache = "NovaMailCache:";
}