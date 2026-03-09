package com.example.novamailservice.service.UserProfile;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Users.UserProfile;

public interface UserProfileService {
    // 新建用户信息
    boolean createUserProfile(UserProfile userProfile);
    // 获取用户信息
    ResponseResult<UserProfile> getUserProfile(Long userId);
}
