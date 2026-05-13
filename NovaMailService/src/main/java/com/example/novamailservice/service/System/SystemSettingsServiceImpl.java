package com.example.novamailservice.service.System;

import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.System.SystemSettings;
import com.example.novamailservice.mapper.System.SystemSettingsMapper;
import com.example.novamailservice.util.Cache;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Service
public class SystemSettingsServiceImpl implements SystemSettingsService {
    private final Logger logger = LoggerFactory.getLogger(SystemSettingsServiceImpl.class.getName());
    private final SystemSettingsMapper systemSettingsMapper;
    private final Cache cache;
    private static final String SystemSettingsCachePrefix = RedisConfig.NovaCache + "SystemSettingsCache:";
    private static final String redisKeyPrefixSettingName = SystemSettingsCachePrefix + "SettingName:";

    @Autowired
    public SystemSettingsServiceImpl(SystemSettingsMapper systemSettingsMapper, Cache cache) {
        this.systemSettingsMapper = systemSettingsMapper;
        this.cache = cache;
    }

    // 类型判断
    private String detectType(String value) {
        if (value == null || value.isEmpty()) return "unknown";

        // Base64
        if (value.startsWith("data:image")) {
            return "base64:image";
        }

        // URL
        if (value.startsWith("http://") || value.startsWith("https://")) {
            return "url";
        }

        // MinIO
        if (value.startsWith("minio://")) {
            return "object";
        }

        // JSON
        if (value.startsWith("{") || value.startsWith("[")) {
            return "json";
        }

        return "text";
    }

    // 写入配置
    private boolean setSystemSettings(String settingName, String settingValue, String settingType) {
        return systemSettingsMapper.setSystemSettings(settingName, settingValue, settingType) > 0;
    }

    // 获取配置
    private ResponseResult<SystemSettings> getSystemSettings(String settingName) {
        String redisKey = redisKeyPrefixSettingName + settingName;

        ResponseResult<SystemSettings> cachedResult = cache.getCache(redisKey, SystemSettings.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 查询数据库
        try {
            SystemSettings systemSettings = systemSettingsMapper.getSystemSettings(settingName);
            if (systemSettings == null) return ResponseResult.error(ErrorCode.NOT_FOUND, "资源不存在");

            // 同步写入缓存
            cache.setCache(redisKey, systemSettings);
            return ResponseResult.success(systemSettings);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public boolean setDefaultAvatar(String settingValue) {
        return setSystemSettings("DefaultAvatar", settingValue, detectType(settingValue));
    }

    @Override
    public ResponseResult<SystemSettings> getDefaultAvatar() {
        return getSystemSettings("DefaultAvatar");
    }

    @Override
    public boolean setDefaultSetting(String settingValue) {
        return setSystemSettings("DefaultSetting", settingValue, "text:html");
    }

    @Override
    public ResponseResult<SystemSettings> getDefaultSetting() {
        return getSystemSettings("DefaultSetting");
    }

}
