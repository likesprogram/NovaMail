package com.example.novamailservice.dto.Mail;

import lombok.Data;

@Data
public class DownloadParsedAttachmentRequest {
    private Long mappingId;
    private Integer partIndex;
}
