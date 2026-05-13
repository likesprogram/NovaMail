package com.example.novamailservice.service.System;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.System.SystemSettings;

public interface SystemSettingsService {
    // 设置默认头像
    boolean setDefaultAvatar(String settingValue);
    // 获取默认头像
    ResponseResult<SystemSettings> getDefaultAvatar();

    // 设置默认用户签名
    boolean setDefaultSetting(String settingValue);
    // 获取用户默认签名
    ResponseResult<SystemSettings> getDefaultSetting();
}
