package com.example.novamailservice.entity.Mail;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class MailBoxFolderState {
    // 邮箱ID
    private Long mailBoxId;

    // 文件夹ID
    private Long folderId;

    // 下一个可分配的 IMAP UID
    private Long uidNext;

    // IMAP UIDVALIDITY
    private Long uidValidity;

    // 创建时间
    private LocalDateTime createTime;

    // 更新时间
    private LocalDateTime updateTime;
}