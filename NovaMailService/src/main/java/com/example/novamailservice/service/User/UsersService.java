package com.example.novamailservice.service.User;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.UpdatePassword;
import com.example.novamailservice.entity.Users.Users;

public interface UsersService {
    boolean createUser(Users user);
    ResponseResult<Users> getCurrentUser(); // 获取当前登录账号
    ResponseResult<Users> getUserByAccount(String account);
    ResponseResult<Users> getUserByMailBoxId(Long mailBoxId);
    ResponseResult<String> updatePassword(Users user, UpdatePassword updatePassword);
    ResponseResult<String> resetPassword(Users user);
    void updateLastLoginTime(Long userId, String account); // 更新登录时间
}
