package com.example.novamailservice.dto.Mail;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class MailDetailStorageInfo {
    private Long mappingId;
    private Long mailBoxId;
    private Long mailId;

    private String senderName;
    private String senderEmail;
    private String subject;
    private String preview;

    private String filePath;
    private String storageBucket;

    private Boolean isRead;
    private Boolean isStarred;
    private Boolean isAnswered;

    private LocalDateTime createTime;
}