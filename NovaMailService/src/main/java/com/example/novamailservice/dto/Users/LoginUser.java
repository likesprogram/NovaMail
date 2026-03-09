package com.example.novamailservice.dto.Users;

import com.example.novamailservice.dto.VerificationCode.ImageCode;
import lombok.Data;

@Data
public class LoginUser {
    // 账号
    private String account;
    // 密码 (明文)
    private String password;
    // 验证码
    private ImageCode imageCode;
}
