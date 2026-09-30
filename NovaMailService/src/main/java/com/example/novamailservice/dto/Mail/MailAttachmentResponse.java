package com.example.novamailservice.dto.Mail;

import lombok.Data;

@Data
public class MailAttachmentResponse {
    private Long attachmentId;
    private String fileName;
    private String fileType;
    private Long fileSize;
    private Boolean isInline;
    private String downloadUrl;
    private Integer partIndex;
    private String sourceType;
    private Long mappingId;
}
