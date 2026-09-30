package com.example.novamailservice.dto.Mail;

import lombok.Data;

import java.util.ArrayList;
import java.util.List;

@Data
public class DraftDetailResponse {
    private Long mappingId;
    private Long mailId;
    private Long mailBoxId;
    private String sendName;
    private String sendMailBoxView;
    private String subject;
    private String bodyData;
    private List<String> toAddressList = new ArrayList<>();
    private List<String> ccAddressList = new ArrayList<>();
    private List<String> bccAddressList = new ArrayList<>();
    private List<SendMailAttachmentRequest> attachments = new ArrayList<>();
}
