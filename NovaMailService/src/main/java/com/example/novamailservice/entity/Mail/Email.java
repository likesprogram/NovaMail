package com.example.novamailservice.entity.Mail;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class Email {
    // 邮件ID
    private Long mailId;
    // 发件人显示名称
    private String senderName;
    // 发件人显示地址
    private String senderEmail;
    // 标题
    private String subject;
    // 内容预览
    private String preview;
    // 存储路径
    private String filePath;
    // 存储桶
    private String storageBucket;
    // 邮件状态
    private String status;
    // 创建时间
    private LocalDateTime createTime;
}
