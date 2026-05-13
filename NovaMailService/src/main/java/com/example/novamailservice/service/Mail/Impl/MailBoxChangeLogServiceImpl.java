package com.example.novamailservice.service.Mail.Impl;

import com.example.novamailservice.entity.Mail.MailBoxChangeLog;
import com.example.novamailservice.mapper.Mail.MailBoxChangeLogMapper;
import com.example.novamailservice.service.Mail.MailBoxChangeLogService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.logging.Logger;

@Service
public class MailBoxChangeLogServiceImpl implements MailBoxChangeLogService {
    private final Logger logger = Logger.getLogger(MailBoxChangeLogServiceImpl.class.getName());
    private final MailBoxChangeLogMapper mailBoxChangeLogMapper;
    @Autowired
    public MailBoxChangeLogServiceImpl(MailBoxChangeLogMapper mailBoxChangeLogMapper) {
        this.mailBoxChangeLogMapper = mailBoxChangeLogMapper;
    }
    @Override
    public boolean writeLog(MailBoxChangeLog log) {
        return mailBoxChangeLogMapper.insert(log) > 0;
    }
}
