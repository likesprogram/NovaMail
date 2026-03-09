package com.example.novamailservice.service.VerificationCode;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.VerificationCode.ImageCode;

import java.awt.*;
import java.io.IOException;

// 验证码服务
public interface VerificationCodeService {
    // 获取图片验证码
    ResponseResult<ImageCode> getImageCode() throws IOException, FontFormatException;
    // 发送短信验证码
    ResponseResult<String> sendSmsCode(String phoneNumber);
    // 发送邮箱验证码
    ResponseResult<String> sendEmailCode(String email);
    // 校验图片验证码
    ResponseResult<String> checkImageCode(ImageCode imageCode);
    // 校验短信验证码
    ResponseResult<String> checkSmsCode(String phoneNumber, String code);
    // 校验邮箱验证码
    ResponseResult<String> checkEmailCode(String email, String code);
}