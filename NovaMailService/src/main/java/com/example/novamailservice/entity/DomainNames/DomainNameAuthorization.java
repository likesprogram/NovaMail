package com.example.novamailservice.entity.DomainNames;

import lombok.Data;

import java.sql.Timestamp;

@Data
public class DomainNameAuthorization {
    // 授权ID
    private Long authorizationId;
    // 域名ID
    private Long domainNameId;
    // 用户ID
    private Long userId;
    // 权限 (USE / ADMIN)
    private String permission;
    // 授权时间
    private Timestamp createTime;
    // 授权过期时间
    private Timestamp expireTime;
}
