package com.example.novamailservice.service.Mail;

import com.example.novamailservice.entity.Mail.MailBoxChangeLog;

public interface MailBoxChangeLogService {
    // 写入日志
    boolean writeLog(MailBoxChangeLog log);
}
