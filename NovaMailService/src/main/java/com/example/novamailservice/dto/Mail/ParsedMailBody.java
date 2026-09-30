package com.example.novamailservice.dto.Mail;

import lombok.Data;

import java.util.ArrayList;
import java.util.List;

@Data
public class ParsedMailBody {
    private String htmlBody;
    private String textBody;
    private List<MailAddressInfo> to = new ArrayList<>();
    private List<MailAddressInfo> cc = new ArrayList<>();
    private List<MailAttachmentResponse> attachments = new ArrayList<>();
}
