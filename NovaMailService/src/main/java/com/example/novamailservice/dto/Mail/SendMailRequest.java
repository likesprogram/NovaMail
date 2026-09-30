package com.example.novamailservice.dto.Mail;

import lombok.Data;

import java.util.List;

@Data
public class SendMailRequest {
    private Long mailBoxId;              // 发件邮箱ID
    private String sendName;             // 发件人显示名称
    private List<String> toAddressList;  // 收件人地址列表
    private List<String> ccAddressList;  // 抄送地址列表
    private List<String> bccAddressList; // 密送地址列表
    private String subject;              // 邮件主题
    private String bodyData;             // 邮件正文 HTML
    private List<SendMailAttachmentRequest> attachments;
    private Long replySourceMappingId;   // 回复来源邮件映射ID
}
