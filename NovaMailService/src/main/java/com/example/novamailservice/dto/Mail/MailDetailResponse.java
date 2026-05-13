package com.example.novamailservice.dto.Mail;

import lombok.Data;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;

@Data
public class MailDetailResponse {
    private Long mappingId;
    private Long mailId;
    private MailAddressInfo from;
    private List<MailAddressInfo> tos = new ArrayList<>();
    private List<MailAddressInfo> ccs = new ArrayList<>();
    private String subject;
    private String preview;
    private String htmlBody;
    private String textBody;

    private Boolean isRead;
    private Boolean isStarred;
    private Boolean isAnswered;

    private LocalDateTime createTime;

    private List<MailAttachmentResponse> attachments;
}