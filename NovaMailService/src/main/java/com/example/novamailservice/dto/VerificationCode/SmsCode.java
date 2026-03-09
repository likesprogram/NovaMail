package com.example.novamailservice.dto.VerificationCode;

import lombok.Data;

@Data
public class SmsCode {
    // 手机号
    private String phoneNumber;
    // 验证码
    private String code;
    // 图片验证码
    private ImageCode imageCode;
}

