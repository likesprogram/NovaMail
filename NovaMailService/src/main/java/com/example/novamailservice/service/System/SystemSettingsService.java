package com.example.novamailservice.service.System;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.System.SystemClientDownloadInfo;
import com.example.novamailservice.entity.System.SystemSettings;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.web.multipart.MultipartFile;

public interface SystemSettingsService {
    // 设置默认头像
    boolean setDefaultAvatar(String settingValue);
    // 获取默认头像
    ResponseResult<SystemSettings> getDefaultAvatar();
    // 上传默认头像
    ResponseResult<SystemSettings> uploadDefaultAvatar(MultipartFile file);

    // 设置默认用户签名
    boolean setDefaultSetting(String settingValue);
    // 获取用户默认签名
    ResponseResult<SystemSettings> getDefaultSetting();

    // 上传客户端安装包
    ResponseResult<SystemClientDownloadInfo> uploadClientPackage(MultipartFile file, String version, String description);
    // 获取客户端安装包信息
    ResponseResult<SystemClientDownloadInfo> getClientDownloadInfo();
    // 下载客户端安装包
    void downloadClientPackage(HttpServletResponse response) throws Exception;
}
