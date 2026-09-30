package com.example.novamailservice.controller;

import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Result.ServiceException;
import com.example.novamailservice.dto.Users.LoginToken;
import com.example.novamailservice.dto.Users.LoginUser;
import com.example.novamailservice.dto.Users.RefreshTokenRequest;
import com.example.novamailservice.dto.Users.RegisterUser;
import com.example.novamailservice.dto.Users.ResetPassword;
import com.example.novamailservice.service.Authentication.AuthenticationService;
import com.example.novamailservice.service.VerificationCode.VerificationCodeService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/authentication")
public class AuthenticationController {
    private final AuthenticationService authenticationService;
    private final VerificationCodeService verificationCodeService;

    @Autowired
    public AuthenticationController(AuthenticationService authenticationService, VerificationCodeService verificationCodeService) {
        this.authenticationService = authenticationService;
        this.verificationCodeService = verificationCodeService;
    }

    @PostMapping("/register")
    public ResponseResult<String> register(@RequestBody RegisterUser registerUser) {
        if (!registerUser.getAccount().equals(registerUser.getSmsCode().getPhoneNumber())) {
            return ResponseResult.error(ErrorCode.USER_PHONE_MISMATCH, "账号与验证码手机号不一致");
        }

        ResponseResult<String> smsCodeResult = verificationCodeService.checkSmsCode(
                registerUser.getSmsCode().getPhoneNumber(),
                registerUser.getSmsCode().getCode()
        );

        if (smsCodeResult.isSuccess()) {
            try {
                return authenticationService.register(registerUser.getAccount(), registerUser.getPassword());
            } catch (ServiceException serviceException) {
                return ResponseResult.error(serviceException.getCode(), serviceException.getMessage());
            } catch (Exception e) {
                return ResponseResult.error(ErrorCode.USER_REGISTER_FAIL, "注册失败");
            }
        }

        return smsCodeResult;
    }

    @PostMapping("/login")
    public ResponseResult<LoginToken> login(@RequestBody LoginUser loginUser) {
        ResponseResult<String> imageCodeResult = verificationCodeService.checkImageCode(loginUser.getImageCode());
        if (imageCodeResult.getCode() != 200) {
            return ResponseResult.error(imageCodeResult.getCode(), imageCodeResult.getMessage());
        }
        return authenticationService.login(loginUser.getAccount(), loginUser.getPassword());
    }

    @PostMapping("/refresh")
    public ResponseResult<LoginToken> refresh(@RequestBody RefreshTokenRequest request) {
        return authenticationService.refreshToken(request == null ? null : request.getRefreshToken());
    }

    @PostMapping("/revokeRefresh")
    public ResponseResult<String> revokeRefresh(@RequestBody RefreshTokenRequest request) {
        return authenticationService.revokeRefreshToken(request == null ? null : request.getRefreshToken());
    }

    @PostMapping("/resetPassword")
    public ResponseResult<String> resetPassword(@RequestBody ResetPassword resetPassword) {
        if (!resetPassword.getAccount().equals(resetPassword.getSmsCode().getPhoneNumber())) {
            return ResponseResult.error(ErrorCode.USER_PHONE_MISMATCH, "账号与验证码手机号不一致");
        }

        ResponseResult<String> smsCodeResult = verificationCodeService.checkSmsCode(
                resetPassword.getSmsCode().getPhoneNumber(),
                resetPassword.getSmsCode().getCode()
        );
        if (!smsCodeResult.isSuccess()) return smsCodeResult;

        return authenticationService.resetPassword(resetPassword.getAccount(), resetPassword.getPassword());
    }

    @PostMapping("/logout")
    public ResponseResult<String> logout() {
        return authenticationService.logout();
    }
}
