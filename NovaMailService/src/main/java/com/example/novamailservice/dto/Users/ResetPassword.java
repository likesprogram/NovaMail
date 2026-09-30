package com.example.novamailservice.dto.Users;

import com.example.novamailservice.dto.VerificationCode.SmsCode;
import lombok.Data;

@Data
public class ResetPassword {
    // 账号
    private String account;
    // 新密码 (明文)
    private String password;
    // 验证码
    private SmsCode smsCode;
}
