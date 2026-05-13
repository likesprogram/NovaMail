package com.example.novamailservice.dto.Users;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class ResultUserProfile {
    // 用户ID
    private Long userId;
    // 账号
    private String account;
    // 是否管理员
    private Boolean isAdmin;   // 默认 false
    // 账号状态
    private String status;
    // 用户昵称
    private String nickname;
    // 用户头像索引
    private Integer avatar;
    // 现居地址
    private String address;
    // 用户简介
    private String resume;
    // 默认发件邮箱
    private Long defaultMailBoxId;
    private String defaultMailBoxView;
    // 最后更新时间
    private LocalDateTime updateTime;
    // 注册时间
    private LocalDateTime registerTime;
    // 最后登录时间
    private LocalDateTime lastLoginTime;
}
