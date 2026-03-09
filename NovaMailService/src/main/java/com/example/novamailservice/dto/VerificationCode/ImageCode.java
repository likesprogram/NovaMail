package com.example.novamailservice.dto.VerificationCode;

import lombok.Data;

@Data
public class ImageCode {
    // 图片验证码
    private String code;
    // 图片验证码的 SessionId
    private String sessionId;
}