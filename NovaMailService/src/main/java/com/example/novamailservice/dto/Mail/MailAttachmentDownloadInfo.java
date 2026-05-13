package com.example.novamailservice.dto.Mail;

import lombok.Data;

@Data
public class MailAttachmentDownloadInfo {
    private Long attachmentId;
    private Long mailId;
    private String fileName;
    private String fileType;
    private Long fileSize;
    private String filePath;
    private String storageBucket;
}
