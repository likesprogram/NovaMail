package com.example.novamailservice.controller;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.VerificationCode.ImageCode;
import com.example.novamailservice.dto.VerificationCode.SmsCode;
import com.example.novamailservice.service.VerificationCode.VerificationCodeService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.awt.*;
import java.io.IOException;

@RestController
@RequestMapping("/verification")
public class VerificationCodeController {
    private final VerificationCodeService verificationCode;

    @Autowired
    public  VerificationCodeController(VerificationCodeService verificationCode) {
        this.verificationCode = verificationCode;
    }

    @PostMapping("/image")
    public ResponseResult<ImageCode> getImageCode() throws IOException, FontFormatException {
        return verificationCode.getImageCode();
    }

    @PostMapping("/sms")
    public ResponseResult<String> sendSmsCode(@RequestBody SmsCode smsCode) {
        // 校验图片验证码
        ResponseResult<String> imageCodeResult = verificationCode.checkImageCode(smsCode.getImageCode());
        if(imageCodeResult.getCode() != 200) return imageCodeResult;

        // 发送短信验证码
        return verificationCode.sendSmsCode(smsCode.getPhoneNumber());
    }
}
