package com.example.novamailservice.entity.Users;

import com.example.novamailservice.dto.Result.ResponseResult;
import lombok.Data;

import java.time.LocalDateTime;
import java.util.regex.Pattern;

@Data
public class UserProfile {
    // 用户ID
    private Long userId;
    // 用户昵称
    private String nickname;
    // 用户头像URL
    private String avatar;
    // 现居地址
    private String address;
    // 用户简介
    private String resume;
    // 默认发件邮箱ID
    private Long defaultMailBoxId;
    // 最后更新时间
    private LocalDateTime updateTime;

    // 用户名校验
    public static ResponseResult<String> validateNickname(String nickname) {
        if (nickname == null || nickname.isEmpty()) return ResponseResult.error("用户名不能为空");
        int nameLength = nickname.codePointCount(0, nickname.length());
        if (nameLength > 64) return ResponseResult.error("用户名长度不能超过64个字符");
        if (nameLength < 2) return ResponseResult.error("用户名长度不能少于2个字符");
        if (!Pattern.compile("^\\X+$").matcher(nickname).matches()) return ResponseResult.error("用户名只能包含可见字符");
        return ResponseResult.success();
    }
}
