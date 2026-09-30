package com.example.novamailservice.entity.Mail;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class EmailAuthorization {
    // 授权ID
    private Long emailAuthorizationId;
    // 用户ID
    private Long userId;
    // SMTP/IMAP 客户端登录使用的授权码
    private String authCode;
    // 授权创建时间
    private LocalDateTime createTime;
    // 授权过期时间
    private LocalDateTime expireTime;
    // 是否启用
    private Integer isActivated;
    // 上次通过该授权成功登录的时间
    private LocalDateTime lastLoginTime;
    // 该授权成功登录次数
    private Integer loginCount;
    // 客户端信息或备注
    private String clientInfo;
    // 最后更新时间
    private LocalDateTime updateTime;
}
