package com.example.novamailservice.service.Mail;

public interface MailNotifyService {
    // 新邮件通知
    public void notifyNewEmail(String account, Long mappingId, Long mailBoxId, Long mailId, Long folderId);

    // 邮件移动通知
    public void notifyMailMoved(String account, Long mappingId, Long fromFolderId, Long toFolderId);

    // 邮件状态更新通知
    public void notifyMailStatusChanged(String account, Long mappingId, String status);
}
