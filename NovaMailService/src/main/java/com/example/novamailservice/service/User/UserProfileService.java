package com.example.novamailservice.service.User;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.HistoryAvatar;
import com.example.novamailservice.entity.Users.UserProfile;
import com.example.novamailservice.entity.Users.Users;

import java.util.List;

public interface UserProfileService {
    // 新建用户信息
    boolean createUserProfile(UserProfile userProfile);
    // 获取用户信息
    ResponseResult<UserProfile> getUserProfileByAccount(String account);
    String getAvatarObjectName(String account);
    // 获取用户历史头像
    ResponseResult<List<HistoryAvatar>> getUserHistoryAvatars(String account);
    // 修改昵称
    ResponseResult<String> updateNickname(Users user, String nickname);
    // 修改用户头像
    ResponseResult<String> updateAvatar(Users user, String base64Data, String fileName);
    // 修改简介
    ResponseResult<String> updateResume(Users user, String resume);
}
