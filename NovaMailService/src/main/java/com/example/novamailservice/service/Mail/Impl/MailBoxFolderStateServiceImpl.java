package com.example.novamailservice.service.Mail.Impl;

import com.example.novamailservice.mapper.Mail.MailBoxFolderStateMapper;
import com.example.novamailservice.service.Mail.MailBoxFolderStateService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Service
public class MailBoxFolderStateServiceImpl implements MailBoxFolderStateService {
    private final MailBoxFolderStateMapper mailBoxFolderStateMapper;

    @Autowired
    public MailBoxFolderStateServiceImpl(MailBoxFolderStateMapper mailBoxFolderStateMapper) {
        this.mailBoxFolderStateMapper = mailBoxFolderStateMapper;
    }

    @Override
    public boolean initSystemFolderStates(Long mailBoxId) {
        return mailBoxFolderStateMapper.initSystemFolderStates(mailBoxId) > 0;
    }
}
