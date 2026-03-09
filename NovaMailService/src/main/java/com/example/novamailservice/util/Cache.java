package com.example.novamailservice.util;

import com.example.novamailservice.config.CacheProper;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.redis.core.RedisCallback;
import org.springframework.data.redis.core.RedisTemplate;
import org.springframework.data.redis.core.ScanOptions;
import org.springframework.stereotype.Component;

import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;

// 缓存
@Component
public class Cache {
    private final Logger logger = LoggerFactory.getLogger(Cache.class.getName());
    private final ObjectMapper objectMapper;
    private final CacheProper cacheProper;
    private final RedisTemplate<String, Object> redisTemplate;

    @Autowired
    public Cache(ObjectMapper objectMapper, CacheProper cacheProper, RedisTemplate<String, Object> redisTemplate) {
        this.objectMapper = objectMapper;
        this.cacheProper = cacheProper;
        this.redisTemplate = redisTemplate;
    }

    // 延长缓存 TTL
    private void extendCacheTTL(String key) {
        try {
            // 获取当前 TTL
            Long currentTtl = redisTemplate.getExpire(key);
            if (currentTtl == null || currentTtl <= 0) return; // key 不存在或已过期

            long newTtl = Math.min(currentTtl + cacheProper.getExtend().getSeconds(),
                    cacheProper.getMax().getSeconds());
            logger.info("延长缓存 TTL [{}]: {} -> {}", key, currentTtl, newTtl);
            redisTemplate.expire(key, Duration.ofSeconds(newTtl));
        } catch (Exception e) {
            logger.error("延长缓存 TTL 异常 [{}]: {}", key, e.getMessage());
        }
    }

    // 设置缓存
    public <T> void setCache(String key, T value) {
        redisTemplate.opsForValue().set(key, value, cacheProper.getDuration());
    }

    // 设置指定时长的缓存
    public <T> void setCache(String key, T value, Duration duration) {
        try {
            redisTemplate.opsForValue().set(key, value, duration);
        } catch (Exception e) {
            logger.error("设置缓存异常 [{}]: {}", key, e.getMessage());
        }
    }

    // 获取缓存
    public <T> ResponseResult<T> getCache(String key, Class<T> clazz) {
        return getCache(key, clazz, true);
    }
    // 获取缓存（指定是否延长时间）
    public <T> ResponseResult<T> getCache(String key, Class<T> clazz, boolean addExtend) {
        try {
            Object cachedObj = redisTemplate.opsForValue().get(key);
            if (cachedObj == null) return ResponseResult.error(ErrorCode.CACHE_MISS, "缓存未命中");

            // 延长过期时间
            if (addExtend) extendCacheTTL(key);

            // 反序列化成目标类型
            T value = objectMapper.convertValue(cachedObj, clazz);
            return ResponseResult.success(value);
        } catch (Exception e) {
            logger.error("Redis 查询异常 [{}]: {}", key, e.getMessage());
            return ResponseResult.error(ErrorCode.CACHE_ERROR, "缓存访问异常");
        }
    }

    // 获取缓存列表
    public <T> ResponseResult<List<T>> getCacheList(String key, Class<T> elementClazz) {
        return getCacheList(key, elementClazz, true);
    }
    // 获取缓存列表（指定是否延长时间）
    public <T> ResponseResult<List<T>> getCacheList(String key, Class<T> elementClazz, boolean addExtend) {
        try {
            Object cachedObj = redisTemplate.opsForValue().get(key);
            if (cachedObj == null) return ResponseResult.error(ErrorCode.CACHE_MISS, "缓存未命中");

            // 延长过期时间
            if (addExtend) extendCacheTTL(key);

            // 反序列化成列表
            List<T> value = objectMapper.convertValue(
                    cachedObj,
                    objectMapper.getTypeFactory().constructCollectionType(List.class, elementClazz)
            );
            return ResponseResult.success(value);
        } catch (Exception e) {
            logger.error("Redis 查询异常 [{}]: {}", key, e.getMessage());
            return ResponseResult.error(ErrorCode.CACHE_ERROR, "缓存访问异常");
        }
    }

    // 删除缓存（单个 key）
    public ResponseResult<Boolean> removeCache(String key) {
        try {
            Boolean result = redisTemplate.delete(key);
            if (Boolean.TRUE.equals(result)) {
                logger.info("缓存已删除 [{}]", key);
                return ResponseResult.success(true);
            } else {
                logger.warn("缓存删除失败或不存在 [{}]", key);
                return ResponseResult.error(ErrorCode.CACHE_MISS, "缓存不存在或已过期");
            }
        } catch (Exception e) {
            logger.error("删除缓存异常 [{}]: {}", key, e.getMessage());
            return ResponseResult.error(ErrorCode.CACHE_ERROR, "缓存删除异常");
        }
    }

    // 批量删除缓存（多个 key）
    public ResponseResult<Long> removeCache(List<String> keys) {
        try {
            Long deletedCount = redisTemplate.delete(keys);
            logger.info("批量删除缓存: 共 {} 条", deletedCount);
            return ResponseResult.success(deletedCount);
        } catch (Exception e) {
            logger.error("批量删除缓存异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.CACHE_ERROR, "批量删除缓存异常");
        }
    }


    // 删除指定前缀的所有缓存
    public ResponseResult<Long> removeCacheByPrefix(String prefix) {
        final int scanCount = 1000;  // SCAN 每次迭代条数
        final int batchSize = 500;   // 批量删除条数
        AtomicLong deletedCount = new AtomicLong(0);

        long startTime = System.currentTimeMillis();

        try {
            redisTemplate.execute((RedisCallback<Void>) connection -> {
                try (var cursor = connection.scan(
                        ScanOptions.scanOptions()
                                .match(prefix + "*")
                                .count(scanCount)
                                .build()
                )) {
                    List<String> batch = new ArrayList<>(batchSize);

                    while (cursor.hasNext()) {
                        batch.add(new String(cursor.next(), StandardCharsets.UTF_8));

                        if (batch.size() >= batchSize) {
                            // 批量删除
                            redisTemplate.delete(batch);
                            deletedCount.addAndGet(batch.size());
                            batch.clear();
                        }
                    }

                    // 删除剩余不足 batchSize 的 key
                    if (!batch.isEmpty()) {
                        redisTemplate.delete(batch);
                        deletedCount.addAndGet(batch.size());
                    }

                } catch (Exception e) {
                    logger.error("Scan 异常 [{}]: {}", prefix, e.getMessage(), e);
                }

                return null;
            });

            long duration = System.currentTimeMillis() - startTime;
            logger.info("按前缀删除缓存 [{}]: 共 {} 条, 耗时 {} ms", prefix, deletedCount.get(), duration);

            return ResponseResult.success(deletedCount.get());

        } catch (Exception e) {
            logger.error("按前缀删除缓存异常 [{}]: {}", prefix, e.getMessage(), e);
            return ResponseResult.error(ErrorCode.CACHE_ERROR, "前缀删除缓存异常");
        }
    }
}
