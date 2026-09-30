package com.example.novamailservice.dto.Users;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class UserFileAttachmentAuthorization {
    private Long attachmentId;
    private Long mailId;
    private String fileName;
    private String fileType;
    private Long fileSize;
    private Boolean isInline;
    private String fileUrl;
    private String mailSubject;
    private String senderName;
    private String senderEmail;
    private LocalDateTime createTime;
}
