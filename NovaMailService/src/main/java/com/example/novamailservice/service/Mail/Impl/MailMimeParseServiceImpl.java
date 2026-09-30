package com.example.novamailservice.service.Mail.Impl;

import com.example.novamailservice.dto.Mail.MailAddressInfo;
import com.example.novamailservice.dto.Mail.MailAttachmentResponse;
import com.example.novamailservice.dto.Mail.ParsedMailBody;
import com.example.novamailservice.service.Mail.MailMimeParseService;
import jakarta.mail.*;
import jakarta.mail.internet.InternetAddress;
import jakarta.mail.internet.MimeMessage;
import jakarta.mail.internet.MimeMultipart;
import jakarta.mail.internet.MimeUtility;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.atomic.AtomicInteger;

@Service
public class MailMimeParseServiceImpl implements MailMimeParseService {
    private static final Logger logger = LoggerFactory.getLogger(MailMimeParseServiceImpl.class);

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
        if (content instanceof MimeMultipart multipart) parseMultipart(multipart, result, new AtomicInteger(0));

        return result;
    }

    private void parseMultipart(MimeMultipart multipart, ParsedMailBody result, AtomicInteger partCounter) throws Exception {
        for (int i = 0; i < multipart.getCount(); i++) {
            BodyPart part = multipart.getBodyPart(i);
            int partIndex = partCounter.getAndIncrement();

            try {
                if (isAttachmentPart(part)) {
                    result.getAttachments().add(toAttachmentResponse(part, partIndex));
                    continue;
                }

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
                    if (content instanceof MimeMultipart childMultipart) parseMultipart(childMultipart, result, partCounter);
                }
            } catch (Exception e) {
                logger.warn("跳过无法解析的 MIME Part，index={}, contentType={}, disposition={}",
                        partIndex, safeContentType(part), safeDisposition(part), e);
            }
        }
    }

    private boolean isAttachmentPart(Part part) throws MessagingException {
        String disposition = part.getDisposition();
        if (Part.ATTACHMENT.equalsIgnoreCase(disposition)) return true;

        String fileName = part.getFileName();
        if (fileName == null || fileName.isBlank()) return false;

        return !Part.INLINE.equalsIgnoreCase(disposition);
    }

    private MailAttachmentResponse toAttachmentResponse(Part part, int partIndex) throws MessagingException {
        MailAttachmentResponse attachment = new MailAttachmentResponse();
        attachment.setAttachmentId(null);
        attachment.setFileName(resolveFileName(part, partIndex));
        attachment.setFileType(resolveContentType(part));
        attachment.setFileSize(resolveFileSize(part));
        attachment.setIsInline(Part.INLINE.equalsIgnoreCase(part.getDisposition()));
        attachment.setPartIndex(partIndex);
        attachment.setSourceType("MIME_PART");
        return attachment;
    }

    private String resolveFileName(Part part, int partIndex) throws MessagingException {
        String fileName = part.getFileName();
        if (fileName != null && !fileName.isBlank()) {
            try {
                fileName = MimeUtility.decodeText(fileName);
            } catch (Exception ignored) {
            }
        }
        if (fileName == null || fileName.isBlank()) fileName = "attachment-" + (partIndex + 1);
        fileName = fileName.replace("\\", "/");
        int slashIndex = fileName.lastIndexOf('/');
        if (slashIndex >= 0) fileName = fileName.substring(slashIndex + 1);
        fileName = fileName.trim();
        return fileName.isBlank() ? "attachment-" + (partIndex + 1) : fileName;
    }

    private String resolveContentType(Part part) throws MessagingException {
        String contentType = part.getContentType();
        if (contentType == null || contentType.isBlank()) return "application/octet-stream";
        int separatorIndex = contentType.indexOf(';');
        if (separatorIndex >= 0) contentType = contentType.substring(0, separatorIndex);
        contentType = contentType.trim();
        return contentType.isBlank() ? "application/octet-stream" : contentType;
    }

    private Long resolveFileSize(Part part) throws MessagingException {
        int size = part.getSize();
        return size < 0 ? 0L : (long) size;
    }

    private String safeContentType(Part part) {
        try {
            return part.getContentType();
        } catch (Exception e) {
            return "unknown";
        }
    }

    private String safeDisposition(Part part) {
        try {
            return part.getDisposition();
        } catch (Exception e) {
            return "unknown";
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
