package com.example.novamailservice.controller;

import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Result.ServiceException;
import com.example.novamailservice.dto.Users.LoginUser;
import com.example.novamailservice.dto.Users.RegisterUser;
import com.example.novamailservice.service.Authentication.AuthenticationService;
import com.example.novamailservice.service.VerificationCode.VerificationCodeService;
import com.example.novamailservice.service.VerificationCode.VerificationCodeServiceImpl;
import lombok.extern.slf4j.Slf4j;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@Slf4j
@RestController
@RequestMapping("/authentication")
public class AuthenticationController {
    private static final Logger logger = LoggerFactory.getLogger(AuthenticationController.class);
    private final AuthenticationService authenticationService;
    private final VerificationCodeService verificationCodeService;
    @Autowired
    public AuthenticationController(AuthenticationService authenticationService, VerificationCodeService verificationCodeService) {
        this.authenticationService = authenticationService;
        this.verificationCodeService = verificationCodeService;
    }

    @PostMapping("/register")
    public ResponseResult<String> register(@RequestBody RegisterUser registerUser) {
        // 验证手机号与账号是否一致
        if (!registerUser.getAccount().equals(registerUser.getSmsCode().getPhoneNumber()))
            return ResponseResult.error(ErrorCode.USER_PHONE_MISMATCH, "账号与验证码手机号不一致");

        // 验证短信验证码
        ResponseResult<String> smsCodeResult = verificationCodeService.checkSmsCode(registerUser.getSmsCode().getPhoneNumber(), registerUser.getSmsCode().getCode());
        if (smsCodeResult.isSuccess()) {
            try {
                return authenticationService.register(registerUser.getAccount(), registerUser.getPassword());
            } catch (ServiceException serviceException) {
                return ResponseResult.error(serviceException.getCode(), serviceException.getMessage());
            } catch (Exception e) {
                return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "未知错误");
            }
        }

        return smsCodeResult;
    }

    @PostMapping("/login")
    public ResponseResult<String> login(@RequestBody LoginUser loginUser) {
        return authenticationService.login(loginUser.getAccount(), loginUser.getPassword());
    }
}
