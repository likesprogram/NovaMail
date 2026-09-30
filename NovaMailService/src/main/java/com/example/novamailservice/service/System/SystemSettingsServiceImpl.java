package com.example.novamailservice.service.System;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.System.SystemClientDownloadInfo;
import com.example.novamailservice.entity.System.SystemSettings;
import com.example.novamailservice.mapper.System.SystemSettingsMapper;
import com.example.novamailservice.util.Cache;
import com.example.novamailservice.util.MinIo.MinIoBucket;
import com.example.novamailservice.util.MinIo.MinIoFactory;
import jakarta.servlet.http.HttpServletResponse;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.web.multipart.MultipartFile;

import java.io.InputStream;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.time.Instant;

@Service
public class SystemSettingsServiceImpl implements SystemSettingsService {
    private final Logger logger = LoggerFactory.getLogger(SystemSettingsServiceImpl.class.getName());
    private final SystemSettingsMapper systemSettingsMapper;
    private final MinIoFactory minIoFactory;
    private final Cache cache;
    private final ObjectMapper objectMapper;
    private static final String SystemSettingsCachePrefix = RedisConfig.NovaCache + "SystemSettingsCache:";
    private static final String redisKeyPrefixSettingName = SystemSettingsCachePrefix + "SettingName:";
    private static final String SYSTEM_BUCKET_NAME = "system";
    private static final String DEFAULT_AVATAR_SETTING = "DefaultAvatar";
    private static final String DEFAULT_SETTING = "DefaultSetting";
    private static final String CLIENT_DOWNLOAD_SETTING = "ClientDownload";

    @Autowired
    public SystemSettingsServiceImpl(SystemSettingsMapper systemSettingsMapper, MinIoFactory minIoFactory, Cache cache, ObjectMapper objectMapper) {
        this.systemSettingsMapper = systemSettingsMapper;
        this.minIoFactory = minIoFactory;
        this.cache = cache;
        this.objectMapper = objectMapper;
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
            return "object:minio";
        }

        // JSON
        if (value.startsWith("{") || value.startsWith("[")) {
            return "json";
        }

        return "text";
    }

    // 写入配置
    private boolean setSystemSettings(String settingName, String settingValue, String settingType) {
        boolean saved = systemSettingsMapper.setSystemSettings(settingName, settingValue, settingType) > 0;
        if (saved) cache.removeCache(redisKeyPrefixSettingName + settingName);
        return saved;
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
        return setSystemSettings(DEFAULT_AVATAR_SETTING, settingValue, detectType(settingValue));
    }

    @Override
    public ResponseResult<SystemSettings> getDefaultAvatar() {
        return getSystemSettings(DEFAULT_AVATAR_SETTING);
    }

    @Override
    public ResponseResult<SystemSettings> uploadDefaultAvatar(MultipartFile file) {
        if (file == null || file.isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "请选择默认头像文件");
        String contentType = file.getContentType();
        if (contentType == null || !contentType.toLowerCase().startsWith("image/")) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "默认头像只支持图片文件");
        }

        String safeFileName = sanitizeFileName(file.getOriginalFilename());
        if (safeFileName == null || safeFileName.isBlank()) safeFileName = "default-avatar.png";
        String objectName = "default-avatar/" + Instant.now().toEpochMilli() + "_" + safeFileName;

        try {
            MinIoBucket bucket = minIoFactory.openBucket(SYSTEM_BUCKET_NAME, true);
            try (InputStream inputStream = file.getInputStream()) {
                bucket.uploadFile(objectName, inputStream, contentType, file.getSize());
            }
            if (!setSystemSettings(DEFAULT_AVATAR_SETTING, "minio://" + SYSTEM_BUCKET_NAME + "/" + objectName, "object:minio")) {
                return ResponseResult.error(ErrorCode.DATABASE_ERROR, "默认头像保存失败");
            }
            return getDefaultAvatar();
        } catch (Exception e) {
            logger.error("默认头像上传失败", e);
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "默认头像上传失败");
        }
    }

    @Override
    public boolean setDefaultSetting(String settingValue) {
        return setSystemSettings(DEFAULT_SETTING, settingValue, "text:html");
    }

    @Override
    public ResponseResult<SystemSettings> getDefaultSetting() {
        return getSystemSettings(DEFAULT_SETTING);
    }

    @Override
    public ResponseResult<SystemClientDownloadInfo> uploadClientPackage(MultipartFile file, String version, String description) {
        if (file == null || file.isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "请选择客户端安装包");

        String safeFileName = sanitizeFileName(file.getOriginalFilename());
        if (safeFileName == null || safeFileName.isBlank()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "客户端文件名不能为空");
        String contentType = file.getContentType();
        if (contentType == null || contentType.isBlank()) contentType = "application/octet-stream";
        String objectName = "client/latest/" + safeFileName;

        try {
            MinIoBucket bucket = minIoFactory.openBucket(SYSTEM_BUCKET_NAME, true);
            try (InputStream inputStream = file.getInputStream()) {
                bucket.uploadFile(objectName, inputStream, contentType, file.getSize());
            }

            SystemClientDownloadInfo info = new SystemClientDownloadInfo();
            info.setFileName(safeFileName);
            info.setFileType(contentType);
            info.setFileSize(file.getSize());
            info.setVersion(limitString(version, 64));
            info.setDescription(limitString(description, 512));
            info.setStorageBucket(SYSTEM_BUCKET_NAME);
            info.setObjectName(objectName);
            info.setUploadTime(Instant.now().toString());

            String json = objectMapper.writeValueAsString(info);
            if (!setSystemSettings(CLIENT_DOWNLOAD_SETTING, json, "json")) {
                return ResponseResult.error(ErrorCode.DATABASE_ERROR, "客户端配置保存失败");
            }
            return ResponseResult.success("上传成功", info);
        } catch (Exception e) {
            logger.error("客户端安装包上传失败", e);
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "客户端安装包上传失败");
        }
    }

    @Override
    public ResponseResult<SystemClientDownloadInfo> getClientDownloadInfo() {
        ResponseResult<SystemSettings> settingsResult = getSystemSettings(CLIENT_DOWNLOAD_SETTING);
        if (!settingsResult.isSuccess()) return ResponseResult.error(settingsResult.getCode(), settingsResult.getMessage());

        try {
            SystemClientDownloadInfo info = objectMapper.readValue(settingsResult.getData().getSettingValue(), SystemClientDownloadInfo.class);
            if (info.getStorageBucket() == null || info.getStorageBucket().isBlank() || info.getObjectName() == null || info.getObjectName().isBlank()) {
                return ResponseResult.error(ErrorCode.NOT_FOUND, "客户端安装包未配置");
            }
            return ResponseResult.success(info);
        } catch (Exception e) {
            logger.error("客户端配置解析失败", e);
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "客户端配置解析失败");
        }
    }

    @Override
    public void downloadClientPackage(HttpServletResponse response) throws Exception {
        ResponseResult<SystemClientDownloadInfo> infoResult = getClientDownloadInfo();
        if (!infoResult.isSuccess()) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }

        SystemClientDownloadInfo info = infoResult.getData();
        MinIoBucket bucket;
        try {
            bucket = minIoFactory.openBucket(info.getStorageBucket());
        } catch (Exception e) {
            response.setStatus(HttpServletResponse.SC_SERVICE_UNAVAILABLE);
            return;
        }
        if (!bucket.fileExists(info.getObjectName())) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }

        String contentType = info.getFileType();
        if (contentType == null || contentType.isBlank()) contentType = bucket.getFileType(info.getObjectName());
        if (contentType == null || contentType.isBlank()) contentType = "application/octet-stream";

        response.setContentType(contentType);
        Long fileSize = info.getFileSize();
        if (fileSize == null || fileSize <= 0) fileSize = bucket.getFileSize(info.getObjectName());
        if (fileSize != null && fileSize > 0) response.setHeader("Content-Length", String.valueOf(fileSize));
        response.setHeader("Content-Disposition", buildContentDisposition(info.getFileName()));
        response.setHeader("X-Content-Type-Options", "nosniff");

        try (InputStream inputStream = bucket.downloadFile(info.getObjectName())) {
            inputStream.transferTo(response.getOutputStream());
        }
    }

    private String sanitizeFileName(String fileName) {
        if (fileName == null) return null;
        String normalized = fileName.replace("\\", "/");
        int slashIndex = normalized.lastIndexOf("/");
        if (slashIndex >= 0) normalized = normalized.substring(slashIndex + 1);
        normalized = normalized.trim();
        if (normalized.length() > 255) normalized = normalized.substring(0, 255);
        return normalized;
    }

    private String limitString(String value, int maxLength) {
        if (value == null) return "";
        String trimmed = value.trim();
        if (trimmed.codePointCount(0, trimmed.length()) <= maxLength) return trimmed;
        int endIndex = trimmed.offsetByCodePoints(0, maxLength);
        return trimmed.substring(0, endIndex);
    }

    private String buildContentDisposition(String fileName) {
        String safeFileName = fileName == null || fileName.isBlank() ? "NovaMail-Client" : fileName;
        String fallbackName = safeFileName.replace("\\", "_").replace("\"", "'");
        fallbackName = fallbackName.chars()
                .mapToObj(ch -> ch >= 0x20 && ch <= 0x7E ? String.valueOf((char) ch) : "_")
                .reduce("", String::concat);
        if (fallbackName.isBlank()) fallbackName = "NovaMail-Client";
        String encodedName = URLEncoder.encode(safeFileName, StandardCharsets.UTF_8).replace("+", "%20");
        return "attachment; filename=\"" + fallbackName + "\"; filename*=UTF-8''" + encodedName;
    }

}
