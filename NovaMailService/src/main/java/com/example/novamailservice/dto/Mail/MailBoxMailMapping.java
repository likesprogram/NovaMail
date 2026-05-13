package com.example.novamailservice.dto.Mail;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class MailBoxMailMapping {
    private Long mappingId;
    private Long mailBoxId;
    private Long mailId;
    private String fromName;
    private String fromMail;
    private String subject;
    private String preview;
    private String status;
    private Long folderId;
    private boolean isRead;
    private boolean isStarred;
    private boolean isAnswered;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
}
