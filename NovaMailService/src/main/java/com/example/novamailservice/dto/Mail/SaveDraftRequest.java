package com.example.novamailservice.dto.Mail;

import lombok.Data;

import java.util.List;

@Data
public class SaveDraftRequest {
    private Long draftMappingId;
    private Long mailBoxId;
    private String sendName;
    private List<String> toAddressList;
    private List<String> ccAddressList;
    private List<String> bccAddressList;
    private String subject;
    private String bodyData;
    private List<SendMailAttachmentRequest> attachments;
}
