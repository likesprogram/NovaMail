package com.example.novamailservice.dto.Users;

import com.example.novamailservice.dto.VerificationCode.SmsCode;
import lombok.Data;

import java.time.LocalDateTime;

@Data
public class RegisterUser {
    // 账号
    private String account;
    // 密码 (明文)
    private String password;
    // 验证码
    private SmsCode smsCode;
}
