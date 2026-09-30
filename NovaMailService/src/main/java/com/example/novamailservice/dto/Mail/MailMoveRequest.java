package com.example.novamailservice.dto.Mail;

import lombok.Data;

@Data
public class MailMoveRequest {
    private Long mappingId;
    private Long targetFolderId;
}
