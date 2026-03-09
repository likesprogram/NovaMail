package com.example.novamailservice.entity.Mail;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class MailBoxChangeLog {
    // 日志ID
    private Long logId;
    // 邮箱ID
    private Long mailBoxId;
    // 操作用户ID
    private Long userId;
    // 操作类型: CREATE, UPDATE, DELETE, DISABLE, ENABLE, TRANSFER
    private String action;
    // 变更前的值快照(JSON)
    private String oldValue;
    // 变更后的值快照(JSON)
    private String newValue;
    // 操作时间
    private LocalDateTime actionTime;
    // 备注信息(可用于记录原因或来源)
    private String remark;
}
