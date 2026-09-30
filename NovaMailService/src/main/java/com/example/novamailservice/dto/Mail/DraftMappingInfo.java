package com.example.novamailservice.dto.Mail;

import lombok.Data;

@Data
public class DraftMappingInfo {
    private Long mappingId;
    private Long mailBoxId;
    private Long mailId;
    private Long folderId;
    private Long imapUid;
    private String filePath;
    private String storageBucket;
}
