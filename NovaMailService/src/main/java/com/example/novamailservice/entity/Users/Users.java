package com.example.novamailservice.entity.Users;

import com.example.novamailservice.dto.Result.ResponseResult;
import java.util.regex.Pattern;
import lombok.Data;

import java.time.LocalDateTime;

@Data
public class Users {
    // 用户Id
    private Long userId;
    // 账号
    private String account;
    // 密码 (密文)
    private String password;
    // 是否管理员
    private Boolean isAdmin;   // 默认 false
    // 账号状态
    private String status;
    // 注册时间
    private LocalDateTime registerTime;
    // 最后登录时间
    private LocalDateTime lastLoginTime;

    // 账号校验
    public static ResponseResult<String> validateAccount(String account) {
        if (account == null || account.isEmpty()) return ResponseResult.error("账号不能为空");
        if (account.length() > 20) return ResponseResult.error("账号长度不能超过20个字符");
        if (account.length() < 11) return ResponseResult.error("账号长度不能少于11个字符");
        if (!account.matches("^[a-zA-Z0-9]+$")) return ResponseResult.error("账号只能包含字母和数字");
        return ResponseResult.success();
    }

    // 密码校验
    public static ResponseResult<String> validatePassword(String password) {
        if (password == null || password.isEmpty()) return ResponseResult.error("密码不能为空");
        if (password.length() > 20) return ResponseResult.error("密码长度不能超过20个字符");
        if (password.length() < 8) return ResponseResult.error("密码长度不能少于8个字符");
        if (!password.matches("^(?=.*[a-z])(?=.*[A-Z])(?=.*\\d)(?=.*[!@#$%^&*()_+\\-=\\[\\]{};':\"\\\\|,.<>/?]).+$"))
            return ResponseResult.error("密码必须包含大写字母、小写字母、数字和特殊字符");
        return ResponseResult.success();
    }
}
