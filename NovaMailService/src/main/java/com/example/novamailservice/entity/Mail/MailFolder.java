package com.example.novamailservice.entity.Mail;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class MailFolder {
    private Long folderId;
    private Long userId;
    private String folderName;
    private String description;
    private boolean isSystem;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
}
