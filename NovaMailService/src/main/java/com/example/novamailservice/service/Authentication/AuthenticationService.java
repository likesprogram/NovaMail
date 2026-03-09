package com.example.novamailservice.service.Authentication;

import com.example.novamailservice.dto.Result.ResponseResult;

// 认证服务
public interface AuthenticationService {
    // 注册
    ResponseResult<String> register(String account, String password);
    // 登录
    ResponseResult<String> login(String account, String password);
}
