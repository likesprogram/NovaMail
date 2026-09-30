package com.example.novamailservice.dto.Mail;

import lombok.Data;

@Data
public class SendMailAttachmentRequest {
    private String fileName;
    private String fileType;
    private Long fileSize;
    private String fileData;
    private String sendType;
}
