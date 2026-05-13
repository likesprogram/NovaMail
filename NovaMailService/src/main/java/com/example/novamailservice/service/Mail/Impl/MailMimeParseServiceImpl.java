package com.example.novamailservice.service.Mail.Impl;

import com.example.novamailservice.dto.Mail.MailAddressInfo;
import com.example.novamailservice.dto.Mail.ParsedMailBody;
import com.example.novamailservice.service.Mail.MailMimeParseService;
import jakarta.mail.*;
import jakarta.mail.internet.InternetAddress;
import jakarta.mail.internet.MimeMessage;
import jakarta.mail.internet.MimeMultipart;
import org.springframework.stereotype.Service;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

@Service
public class MailMimeParseServiceImpl implements MailMimeParseService {

    @Override
    public ParsedMailBody parse(InputStream inputStream) throws Exception {
        Session session = Session.getDefaultInstance(new Properties());
        MimeMessage message = new MimeMessage(session, inputStream);

        ParsedMailBody result = new ParsedMailBody();
        Object content = message.getContent();
        // 收件人
        result.setTo(parseAddressList(message.getRecipients(Message.RecipientType.TO)));
        // 抄送
        result.setCc(parseAddressList(message.getRecipients(Message.RecipientType.CC)));
        if (content instanceof String text) {
            if (message.isMimeType("text/html")) result.setHtmlBody(text);
            else result.setTextBody(text);
            return result;
        }

        if (content instanceof MimeMultipart multipart) parseMultipart(multipart, result);

        return result;
    }

    private void parseMultipart(MimeMultipart multipart, ParsedMailBody result) throws Exception {
        for (int i = 0; i < multipart.getCount(); i++) {
            BodyPart part = multipart.getBodyPart(i);

            if (part.isMimeType("text/html")) {
                Object content = part.getContent();
                if (content instanceof String html && result.getHtmlBody() == null) result.setHtmlBody(html);
                continue;
            }

            if (part.isMimeType("text/plain")) {
                Object content = part.getContent();
                if (content instanceof String text && result.getTextBody() == null) result.setTextBody(text);
                continue;
            }

            if (part.isMimeType("multipart/*")) {
                Object content = part.getContent();
                if (content instanceof MimeMultipart childMultipart) parseMultipart(childMultipart, result);
            }
        }
    }

    private List<MailAddressInfo> parseAddressList(Address[] addresses) {
        List<MailAddressInfo> result = new ArrayList<>();

        if (addresses == null) return result;

        for (Address address : addresses) {
            MailAddressInfo info = new MailAddressInfo();

            if (address instanceof InternetAddress internetAddress) {
                info.setAddress(internetAddress.getAddress());

                String personal = internetAddress.getPersonal();

                if (personal == null || personal.isBlank()) {
                    info.setName(internetAddress.getAddress());
                } else {
                    info.setName(personal);
                }
            } else {
                info.setAddress(address.toString());
                info.setName(address.toString());
            }

            result.add(info);
        }

        return result;
    }
}
