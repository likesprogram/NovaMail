package com.example.novamailservice.service.Authentication;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.LoginToken;

public interface AuthenticationService {
    ResponseResult<String> register(String account, String password);
    ResponseResult<LoginToken> login(String account, String password);
    ResponseResult<LoginToken> refreshToken(String refreshToken);
    ResponseResult<String> revokeRefreshToken(String refreshToken);
    ResponseResult<String> resetPassword(String account, String password);
    ResponseResult<String> logout();
}
