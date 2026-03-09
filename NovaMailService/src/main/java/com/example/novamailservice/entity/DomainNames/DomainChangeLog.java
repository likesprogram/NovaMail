package com.example.novamailservice.entity.DomainNames;

import lombok.Data;

import java.sql.Timestamp;

@Data
public class DomainChangeLog {
    // 日志ID
    private Long logId;
    // 域名ID
    private Long domainNameId;
    // 操作用户ID
    private Long userId;
    // 变更类型
    private String changeType;
    // 详细描述
    private String description;
    // 变更时间
    private Timestamp changeTime;
    // 操作来源IP
    private String ipAddress;
    // 操作客户端信息
    private String userAgent;
    // 变更结果 (SUCCESS / FAIL)
    private String changeResult;
    // 失败原因
    private String failReason;
}
