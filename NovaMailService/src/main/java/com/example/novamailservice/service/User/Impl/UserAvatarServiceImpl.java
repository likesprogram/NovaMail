package com.example.novamailservice.service.User.Impl;

import com.aliyun.core.utils.IOUtils;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.System.SystemSettings;
import com.example.novamailservice.service.System.SystemSettingsService;
import com.example.novamailservice.service.User.UserAvatarService;
import com.example.novamailservice.service.User.UserFilesService;
import com.example.novamailservice.service.User.UserProfileService;
import com.example.novamailservice.util.MinIo.MinIoBucket;
import com.example.novamailservice.util.MinIo.MinIoFactory;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.stereotype.Service;

import java.io.InputStream;
import java.net.URI;
import java.util.Base64;

@Service
public class UserAvatarServiceImpl implements UserAvatarService {
    private final UserProfileService userProfileService;
    private final MinIoFactory minIoFactory;
    private final UserFilesService userFilesService;
    private final SystemSettingsService systemSettingsService;

    public UserAvatarServiceImpl(UserProfileService userProfileService, MinIoFactory minIoFactory, UserFilesService userFilesService, SystemSettingsService systemSettingsService) {
        this.userProfileService = userProfileService;
        this.minIoFactory = minIoFactory;
        this.userFilesService = userFilesService;
        this.systemSettingsService = systemSettingsService;
    }

    // 判断 文件类型
    private String guessImageContentType(String objectName) {
        if (objectName == null) return "image/png";
        String lowerName = objectName.toLowerCase();
        if (lowerName.endsWith(".png")) return "image/png";
        if (lowerName.endsWith(".jpg") || lowerName.endsWith(".jpeg")) return "image/jpeg";
        if (lowerName.endsWith(".gif")) return "image/gif";
        if (lowerName.endsWith(".webp")) return "image/webp";
        return "image/png";
    }

    // 从 MinIo 中下载头像
    private void writeMinioObjectToResponse(String bucketName, String objectName, String defaultFileName, HttpServletResponse response) throws Exception {
        if (bucketName == null || bucketName.isBlank()) {
            response.setStatus(HttpServletResponse.SC_SERVICE_UNAVAILABLE);
            return;
        }
        if (objectName == null || objectName.isBlank()) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }
        MinIoBucket minIoBucket;
        try {
            minIoBucket = minIoFactory.openBucket(bucketName);
        } catch (Exception e) {
            response.setStatus(HttpServletResponse.SC_SERVICE_UNAVAILABLE);
            return;
        }
        if (!minIoBucket.fileExists(objectName)) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }
        try (InputStream inputStream = minIoBucket.downloadFile(objectName)) {
            String contentType = minIoBucket.getFileType(objectName);
            if (contentType == null || contentType.equals("application/octet-stream")) contentType = guessImageContentType(objectName);
            response.setContentType(contentType);
            String ext = "";
            int dotIndex = objectName.lastIndexOf(".");
            if (dotIndex != -1) ext = objectName.substring(dotIndex);
            String fileName = defaultFileName + ext;
            response.setHeader("Content-Disposition", "inline; filename=\"" + fileName + "\"");
            IOUtils.copy(inputStream, response.getOutputStream());
        } catch (Exception e) {
            response.setStatus(HttpServletResponse.SC_SERVICE_UNAVAILABLE);
        }
    }

    @Override
    public void writeAvatarByAccount(String account, HttpServletResponse response) throws Exception {
        if (account == null || account.isBlank()) {
            writeDefaultAvatar(response);
            return;
        }
        // 默认桶
        String bucketName = userFilesService.getStorageBucketName();
        // 用户头像路径
        String userAvatarPath = userProfileService.getAvatarObjectName(account);
        // 判断用户头像是否存在
        if (userAvatarPath == null || userAvatarPath.isEmpty()) {
            writeDefaultAvatar(response);
            return;
        }
        writeMinioObjectToResponse(bucketName, userAvatarPath, "avatar", response);
    }

    @Override
    public void writeDefaultAvatar(HttpServletResponse response) throws Exception {
        ResponseResult<SystemSettings> systemSettings = systemSettingsService.getDefaultAvatar();
        if (!systemSettings.isSuccess()) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }

        String value = systemSettings.getData().getSettingValue();
        String type = systemSettings.getData().getSettingType();

        if ("url".equals(type)) {
            response.sendRedirect(value);
            return;
        }
        if (type != null && type.startsWith("base64")) {
            try {
                int commaIndex = value.indexOf(',');

                if (commaIndex < 0) {
                    response.setStatus(HttpServletResponse.SC_NOT_FOUND);
                    return;
                }

                String meta = value.substring(0, commaIndex);
                String base64Data = value.substring(commaIndex + 1);

                String contentType = meta.substring(5, meta.indexOf(';'));
                byte[] bytes = Base64.getDecoder().decode(base64Data);

                response.setContentType(contentType);
                response.setHeader("Content-Disposition", "inline; filename=\"avatar.png\"");
                response.getOutputStream().write(bytes);
                return;
            } catch (Exception e) {
                response.setStatus(HttpServletResponse.SC_SERVICE_UNAVAILABLE);
                return;
            }
        }
        if ("object:minio".equals(type)) {
            try {
                URI uri = URI.create(value);

                if (!"minio".equals(uri.getScheme())) {
                    response.setStatus(HttpServletResponse.SC_SERVICE_UNAVAILABLE);
                    return;
                }

                String bucketName = uri.getHost();
                String path = uri.getPath();

                if (bucketName == null || bucketName.isBlank() || path == null || path.length() <= 1) {
                    response.setStatus(HttpServletResponse.SC_SERVICE_UNAVAILABLE);
                    return;
                }

                String objectName = path.substring(1);
                writeMinioObjectToResponse(bucketName, objectName, "avatar", response);
                return;
            } catch (Exception e) {
                response.setStatus(HttpServletResponse.SC_SERVICE_UNAVAILABLE);
                return;
            }
        }
        response.setStatus(HttpServletResponse.SC_NOT_FOUND);
    }
}
