package com.example.novamailservice.service.Mail;

import com.example.novamailservice.dto.Mail.ParsedMailBody;

import java.io.InputStream;

public interface MailMimeParseService {
    ParsedMailBody parse(InputStream inputStream) throws Exception;
}
