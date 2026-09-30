package com.example.novamailservice.dto.Mail;

import lombok.Data;

@Data
public class SaveDraftResponse {
    private Long mappingId;
    private Long mailId;
    private Long mailBoxId;
    private Long folderId;
    private Long imapUid;
}
