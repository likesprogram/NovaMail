package com.example.novamailservice.entity.DomainNames;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class DomainNames {
    // 域名ID
    private Long domainNameId;
    // 域名
    private String domainName;
    // 用户ID（null 表示公共域名）
    private Long userId;
    // 是否有 TLS 证书
    private Boolean hasCertificate;
    // 是否有 DKIM 密钥
    private Boolean hasDKIM;
    // 是否默认域名
    private Boolean isDefault;
    // 最后更新时间
    private LocalDateTime updateTime;
}
