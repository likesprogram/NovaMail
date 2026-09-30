package com.example.novamailservice.service.Mail.Impl;

import com.example.novamailservice.dto.Mail.*;
import com.example.novamailservice.dto.MailBoxAddressToMailBoxAndUser;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Mail.Email;
import com.example.novamailservice.entity.Mail.MailBoxes;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.mapper.Mail.EmailMapper;
import com.example.novamailservice.service.Mail.EmailService;
import com.example.novamailservice.service.Mail.MailBoxesService;
import com.example.novamailservice.service.Mail.MailMimeParseService;
import com.example.novamailservice.service.Mail.MailNotifyService;
import com.example.novamailservice.service.User.UserFilesService;
import com.example.novamailservice.service.User.UsersService;
import com.example.novamailservice.util.MinIo.MinIoBucket;
import com.example.novamailservice.util.MinIo.MinIoFactory;
import com.example.novamailservice.util.NovaMailHub.Message.*;
import com.example.novamailservice.util.NovaMailHub.NovaMailHubClient;
import org.jetbrains.annotations.NotNull;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.transaction.interceptor.TransactionAspectSupport;

import jakarta.servlet.http.HttpServletResponse;
import java.io.ByteArrayInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneOffset;
import java.time.format.DateTimeFormatter;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import jakarta.mail.BodyPart;
import jakarta.mail.Address;
import jakarta.mail.Message;
import jakarta.mail.Part;
import jakarta.mail.Session;
import jakarta.mail.internet.MimeBodyPart;
import jakarta.mail.internet.MimeUtility;
import jakarta.mail.internet.InternetAddress;
import jakarta.mail.internet.MimeMessage;
import jakarta.mail.internet.MimeMultipart;
import java.io.ByteArrayOutputStream;
import java.util.Properties;

@Service
public class EmailServiceImpl implements EmailService {
    private static final Pattern USER_FILE_LINK_PATTERN = Pattern.compile("(?:https?://[^\"'\\s<>]+)?/?userFiles/(?:download|preview)\\?([^\"'\\s<>]+)", Pattern.CASE_INSENSITIVE);
    private static final long SNOWFLAKE_EPOCH = 1609459200000L;
    private static final long SNOWFLAKE_WORKER_ID = 1023L;
    private static final long SNOWFLAKE_SEQUENCE_MASK = 0xFFFL;
    private static final int SNOWFLAKE_WORKER_SHIFT = 12;
    private static final int SNOWFLAKE_TIMESTAMP_SHIFT = 22;
    private static final long DRAFT_FOLDER_ID = 1L;
    private static final long INBOX_FOLDER_ID = 2L;
    private static final long TRASH_FOLDER_ID = 4L;
    private static final long SPAM_FOLDER_ID = 5L;
    private static final long RECYCLE_FOLDER_ID = 6L;
    private static final long DELETED_FOLDER_ID = 7L;
    private static final String MAIL_STORAGE_BUCKET = "mails";
    private static final int SUBJECT_MAX_LENGTH = 255;
    private static final int SENDER_NAME_MAX_LENGTH = 255;
    private static final int PREVIEW_MAX_LENGTH = 1024;
    private static final int STORAGE_PATH_MAX_LENGTH = 255;
    private static final DateTimeFormatter MAIL_PATH_DATE_FORMATTER = DateTimeFormatter.ofPattern("yyyy/MM/dd");
    private static final Object MAIL_ID_LOCK = new Object();
    private static long lastMailTimestamp = -1L;
    private static long mailIdSequence = 0L;

    private final Logger logger = LoggerFactory.getLogger(EmailServiceImpl.class.getName());
    private final EmailMapper emailMapper;
    private final UsersService usersService;
    private final NovaMailHubClient novaMailHubClient;
    private final MailNotifyService mailNotifyService;
    private final MailBoxesService mailBoxesService;
    private final MailMimeParseService mailMimeParseService;
    private final MinIoFactory minIoFactory;
    private final UserFilesService userFilesService;

    @Autowired
    public EmailServiceImpl(EmailMapper emailMapper, UsersService usersService, NovaMailHubClient novaMailHubClient, MailNotifyService mailNotifyService, MailBoxesService mailBoxesService, MailMimeParseService mailMimeParseService, MinIoFactory minIoFactory, UserFilesService userFilesService) {
        this.emailMapper = emailMapper;
        this.usersService = usersService;
        this.novaMailHubClient = novaMailHubClient;
        this.mailNotifyService = mailNotifyService;
        this.mailBoxesService = mailBoxesService;
        this.mailMimeParseService = mailMimeParseService;
        this.minIoFactory = minIoFactory;
        this.userFilesService = userFilesService;
    }

    private byte[] buildRawMimeBytes(String fromAddress, String sendName, List<String> toAddresses, String subject, String htmlBody, List<SendMailAttachmentRequest> attachments) throws Exception {
        return buildRawMimeBytes(fromAddress, sendName, toAddresses, List.of(), List.of(), subject, htmlBody, attachments);
    }

    private byte[] buildRawMimeBytes(String fromAddress, String sendName, List<String> toAddresses, List<String> ccAddresses, List<String> bccAddresses, String subject, String htmlBody, List<SendMailAttachmentRequest> attachments) throws Exception {
        Properties props = new Properties();
        Session session = Session.getInstance(props);

        MimeMessage message = new MimeMessage(session);
        InternetAddress from = new InternetAddress(fromAddress, sendName == null ? "" : sendName, java.nio.charset.StandardCharsets.UTF_8.name());
        message.setFrom(from);
        for (String to : toAddresses) message.addRecipient(Message.RecipientType.TO, new InternetAddress(to));
        for (String cc : ccAddresses) message.addRecipient(Message.RecipientType.CC, new InternetAddress(cc));
        for (String bcc : bccAddresses) message.addRecipient(Message.RecipientType.BCC, new InternetAddress(bcc));

        message.setSubject(subject == null ? "（无题）" : subject, java.nio.charset.StandardCharsets.UTF_8.name());
        List<SendMailAttachmentRequest> mimeAttachments = normalizeMimeAttachments(attachments);
        if (mimeAttachments.isEmpty()) {
            message.setContent(htmlBody == null ? "" : htmlBody, "text/html;charset=UTF-8");
        } else {
            MimeMultipart mixed = new MimeMultipart("mixed");

            MimeBodyPart htmlPart = new MimeBodyPart();
            htmlPart.setContent(htmlBody == null ? "" : htmlBody, "text/html;charset=UTF-8");
            mixed.addBodyPart(htmlPart);

            for (SendMailAttachmentRequest attachment : mimeAttachments) {
                MimeBodyPart attachmentPart = new MimeBodyPart();
                String fileName = sanitizeAttachmentFileName(attachment.getFileName());
                String encodedFileName = MimeUtility.encodeText(fileName, StandardCharsets.UTF_8.name(), null);
                String fileType = normalizeAttachmentContentType(attachment.getFileType());
                byte[] fileBytes = decodeBase64Payload(attachment.getFileData());
                attachmentPart.setDataHandler(new jakarta.activation.DataHandler(new ByteArrayAttachmentDataSource(fileBytes, fileType, fileName)));
                attachmentPart.setDisposition(jakarta.mail.Part.ATTACHMENT);
                attachmentPart.setFileName(encodedFileName);
                mixed.addBodyPart(attachmentPart);
            }

            message.setContent(mixed);
        }
        message.setSentDate(new java.util.Date());
        message.saveChanges();

        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        message.writeTo(outputStream);
        return outputStream.toByteArray();
    }

    private List<SendMailAttachmentRequest> normalizeMimeAttachments(List<SendMailAttachmentRequest> attachments) {
        if (attachments == null || attachments.isEmpty()) return List.of();
        return attachments.stream()
                .filter(item -> item != null && "MIME_ATTACHMENT".equals(item.getSendType()))
                .filter(item -> item.getFileData() != null && !item.getFileData().isBlank())
                .toList();
    }

    private byte[] decodeBase64Payload(String dataUrl) {
        String value = dataUrl == null ? "" : dataUrl.trim();
        int commaIndex = value.indexOf(',');
        if (commaIndex >= 0) value = value.substring(commaIndex + 1);
        return Base64.getDecoder().decode(value);
    }

    private String sanitizeAttachmentFileName(String fileName) {
        if (fileName == null || fileName.isBlank()) return "attachment";
        String normalized = fileName.replace("\\", "/");
        int slashIndex = normalized.lastIndexOf('/');
        if (slashIndex >= 0) normalized = normalized.substring(slashIndex + 1);
        normalized = normalized.trim();
        if (normalized.isBlank()) return "attachment";
        return normalized.length() > 255 ? normalized.substring(0, 255) : normalized;
    }

    private String normalizeAttachmentContentType(String fileType) {
        if (fileType == null || fileType.isBlank()) return "application/octet-stream";
        return fileType.length() > 100 ? "application/octet-stream" : fileType;
    }

    private record ByteArrayAttachmentDataSource(byte[] bytes, String contentType, String name) implements jakarta.activation.DataSource {
        @Override
        public InputStream getInputStream() {
            return new java.io.ByteArrayInputStream(bytes);
        }

        @Override
        public java.io.OutputStream getOutputStream() {
            throw new UnsupportedOperationException("Attachment data source is read-only");
        }

        @Override
        public String getContentType() {
            return contentType;
        }

        @Override
        public String getName() {
            return name;
        }
    }

    private Map<String, Object> buildMailBoxMetadata(MailBoxes mailBox, Users user) {
        Map<String, Object> mailbox = new HashMap<>();
        mailbox.put("mailBoxId", String.valueOf(mailBox.getMailBoxId()));
        mailbox.put("userId", String.valueOf(mailBox.getUserId()));
        mailbox.put("domainNameId", String.valueOf(mailBox.getDomainNameId()));
        mailbox.put("localPart", mailBox.getLocalPart());
        // 是否启用，当前邮箱
        mailbox.put("enabled", mailBox.getIsOpen() != null && mailBox.getIsOpen() == 1);
        mailbox.put("userStatus", toUserStatusValue(user));
        return mailbox;
    }
    private Map<String, Object> buildEmptyMailBoxMetadata() {
        Map<String, Object> mailbox = new HashMap<>();
        mailbox.put("mailBoxId", "0");
        mailbox.put("userId", "0");
        mailbox.put("domainNameId", "0");
        mailbox.put("localPart", "");
        mailbox.put("enabled", false);
        // UserStatus::Normal = 0
        mailbox.put("userStatus", 0);
        return mailbox;
    }
    private int toUserStatusValue(Users user) {
        if (user == null || user.getStatus() == null) return 0;
        String status = String.valueOf(user.getStatus());
        return switch (status) {
            case "NORMAL" -> 0;
            case "DISABLED" -> 1;
            case "DELETED" -> 2;
            default -> 0;
        };
    }
    private Map<String, Object> buildOutboundRcptToMetadata(String address) {
        Map<String, Object> rcpt = new HashMap<>();

        rcpt.put("mailbox", buildEmptyMailBoxMetadata());
        rcpt.put("address", address);
        rcpt.put("addressString", address);
        rcpt.put("displayName", "");

        rcpt.put("notify", 0);
        rcpt.put("orcpt", "");
        rcpt.put("extensions", new HashMap<String, String>());

        // RcptTargetType::Outbound = 2
        rcpt.put("targetType", 2);

        return rcpt;
    }
    private Map<String, Object> buildLocalRcptToMetadata(String address, MailBoxAddressToMailBoxAndUser info) {
        Map<String, Object> rcpt = new HashMap<>();
        rcpt.put("mailbox", buildMailBoxMetadata(info.getMailBox(), info.getUser()));
        rcpt.put("address", address);
        rcpt.put("addressString", address);
        rcpt.put("displayName", "");

        // RcptNotify::None = 0
        rcpt.put("notify", 0);
        rcpt.put("orcpt", "");
        rcpt.put("extensions", new HashMap<String, String>());

        // RcptTargetType::Local = 1
        rcpt.put("targetType", 1);
        return rcpt;
    }
    private Map<String, Object> buildMailFromMetadata(MailBoxes mailBox, Users user, String fromAddress, String sendName, int mailSize) {
        Map<String, Object> mailFrom = new HashMap<>();
        mailFrom.put("mailbox", buildMailBoxMetadata(mailBox, user));
        mailFrom.put("address", fromAddress);
        mailFrom.put("addressString", fromAddress);
        mailFrom.put("displayName", sendName == null ? "" : sendName);

        // C++ Configuration 没有 uint64_t，mailSize 按字符串传
        mailFrom.put("mailSize", String.valueOf(mailSize));

        // MailBody::Unspecified = 0
        mailFrom.put("body", 0);

        // MailSourceType::Local = 1
        mailFrom.put("sourceType", 1);
        mailFrom.put("nullReversePath", false);
        return mailFrom;
    }
    private String normalizeEmailAddress(String rawAddress) {
        if (rawAddress == null || rawAddress.isBlank()) return null;
        try {
            InternetAddress address = new InternetAddress(rawAddress.trim(), false);
            address.validate();
            String value = address.getAddress();
            if (value == null || value.isBlank()) return null;
            return value.trim();
        } catch (Exception e) { return null; }
    }

    private List<String> normalizeEmailAddresses(Collection<String> rawAddresses) {
        if (rawAddresses == null || rawAddresses.isEmpty()) return List.of();
        Set<String> normalizedAddresses = new LinkedHashSet<>();
        for (String rawAddress : rawAddresses) {
            if (rawAddress == null || rawAddress.isBlank()) continue;
            String address = normalizeEmailAddress(rawAddress);
            if (address == null || address.isBlank()) throw new IllegalArgumentException("邮箱地址格式错误：" + rawAddress);
            normalizedAddresses.add(address);
        }
        return new ArrayList<>(normalizedAddresses);
    }

    private List<String> normalizeDraftAddressList(Collection<String> rawAddresses) {
        try {
            return normalizeEmailAddresses(rawAddresses);
        } catch (IllegalArgumentException e) {
            throw e;
        }
    }

    private String safeText(String value) {
        return value == null ? "" : value;
    }

    private String trimToLength(String value, int maxLength) {
        String text = safeText(value);
        if (text.length() <= maxLength) return text;
        return text.substring(0, maxLength);
    }

    private String htmlToPreview(String htmlBody) {
        String text = safeText(htmlBody)
                .replaceAll("(?is)<style[^>]*>.*?</style>", " ")
                .replaceAll("(?is)<script[^>]*>.*?</script>", " ")
                .replaceAll("(?is)<[^>]+>", " ")
                .replace("&nbsp;", " ")
                .replace("&lt;", "<")
                .replace("&gt;", ">")
                .replace("&amp;", "&")
                .replace("&quot;", "\"")
                .replace("&#39;", "'");
        text = text.replaceAll("\\s+", " ").trim();
        return trimToLength(text, PREVIEW_MAX_LENGTH);
    }

    private long nextMailId() {
        synchronized (MAIL_ID_LOCK) {
            long timestamp = currentMailTimestamp();
            if (timestamp < lastMailTimestamp) timestamp = waitUntilMailTimestamp(lastMailTimestamp);

            if (timestamp == lastMailTimestamp) {
                mailIdSequence = (mailIdSequence + 1) & SNOWFLAKE_SEQUENCE_MASK;
                if (mailIdSequence == 0) timestamp = waitNextMailTimestamp(lastMailTimestamp);
            } else {
                mailIdSequence = 0L;
            }

            lastMailTimestamp = timestamp;
            return ((timestamp - SNOWFLAKE_EPOCH) << SNOWFLAKE_TIMESTAMP_SHIFT)
                    | (SNOWFLAKE_WORKER_ID << SNOWFLAKE_WORKER_SHIFT)
                    | mailIdSequence;
        }
    }

    private long currentMailTimestamp() {
        return Math.max(SNOWFLAKE_EPOCH, Instant.now().toEpochMilli());
    }

    private long waitNextMailTimestamp(long lastTimestamp) {
        long timestamp;
        do {
            timestamp = currentMailTimestamp();
        } while (timestamp <= lastTimestamp);
        return timestamp;
    }

    private long waitUntilMailTimestamp(long targetTimestamp) {
        long timestamp;
        do {
            timestamp = currentMailTimestamp();
        } while (timestamp < targetTimestamp);
        return timestamp;
    }

    private String buildMailPath(long mailId) {
        long timestamp = (mailId >> 22) + SNOWFLAKE_EPOCH;
        String datePath = LocalDateTime.ofInstant(Instant.ofEpochMilli(timestamp), ZoneOffset.UTC).format(MAIL_PATH_DATE_FORMATTER);
        long hash = mailId;
        int b1 = (int) (hash & 0xFF);
        int b2 = (int) ((hash >> 8) & 0xFF);
        String path = "%s/%02x%02x%016x.eml".formatted(datePath, b1, b2, mailId);
        if (path.length() > STORAGE_PATH_MAX_LENGTH) throw new IllegalStateException("邮件存储路径过长");
        return path;
    }

    private long allocateImapUid(Long mailBoxId, Long folderId) {
        emailMapper.ensureMailBoxFolderState(mailBoxId, folderId);
        Long uidNext = emailMapper.getUidNextForUpdate(mailBoxId, folderId);
        if (uidNext == null || uidNext <= 0 || uidNext > 0xFFFFFFFFL) throw new IllegalStateException("IMAP UID 已耗尽或无效");
        if (emailMapper.increaseUidNext(mailBoxId, folderId) <= 0) throw new IllegalStateException("IMAP UID 分配失败");
        return uidNext;
    }

    private MailBoxes requireOwnedMailBox(Users user, Long mailBoxId) {
        if (mailBoxId == null || mailBoxId <= 0) throw new IllegalArgumentException("发件邮箱不能为空");
        ResponseResult<MailBoxes> mailBoxResult = mailBoxesService.getMailBoxById(mailBoxId);
        if (!mailBoxResult.isSuccess() || mailBoxResult.getData() == null) throw new IllegalArgumentException("发件邮箱不存在");
        MailBoxes mailBox = mailBoxResult.getData();
        if (!Objects.equals(mailBox.getUserId(), user.getUserId())) throw new SecurityException("无权使用该发件邮箱");
        return mailBox;
    }

    private String requireMailBoxAddress(Long mailBoxId) {
        ResponseResult<String> addressResult = mailBoxesService.getMailBoxAddress(mailBoxId);
        if (!addressResult.isSuccess() || addressResult.getData() == null || addressResult.getData().isBlank()) {
            throw new IllegalArgumentException("发件邮箱地址不存在");
        }
        return addressResult.getData();
    }

    private String replaceUserFileLinksWithSharedLinks(Users user, String htmlBody, String baseUrl) {
        if (htmlBody == null || htmlBody.isBlank()) return htmlBody;

        Matcher matcher = USER_FILE_LINK_PATTERN.matcher(htmlBody);
        StringBuffer result = new StringBuffer();
        while (matcher.find()) {
            Long fileId = parseFileIdFromQuery(matcher.group(1));
            String matchedLink = matcher.group(0);
            String sharedBaseUrl = resolveSharedLinkBaseUrl(matchedLink, baseUrl);
            String replacement = fileId == null ? matchedLink : userFilesService.createSharedDownloadUrl(user, fileId, sharedBaseUrl);
            if (replacement == null || replacement.isBlank()) replacement = matcher.group(0);
            matcher.appendReplacement(result, Matcher.quoteReplacement(replacement));
        }
        matcher.appendTail(result);
        return result.toString();
    }

    private String resolveSharedLinkBaseUrl(String matchedLink, String fallbackBaseUrl) {
        String baseFromLink = extractBaseUrlFromUserFileLink(matchedLink);
        if (baseFromLink != null && !baseFromLink.isBlank()) return baseFromLink;
        if (fallbackBaseUrl == null || fallbackBaseUrl.isBlank()) return "";
        return fallbackBaseUrl.trim().replaceAll("/+$", "");
    }

    private String extractBaseUrlFromUserFileLink(String matchedLink) {
        if (matchedLink == null || matchedLink.isBlank()) return "";
        String lowerLink = matchedLink.toLowerCase(Locale.ROOT);
        int userFilesIndex = lowerLink.indexOf("/userfiles/");
        if (userFilesIndex <= 0) return "";
        String baseUrl = matchedLink.substring(0, userFilesIndex).trim();
        return baseUrl.replaceAll("/+$", "");
    }

    private Long parseFileIdFromQuery(String query) {
        if (query == null || query.isBlank()) return null;
        String[] params = query.split("&");
        for (String param : params) {
            int equalIndex = param.indexOf('=');
            if (equalIndex < 0) continue;
            String key = param.substring(0, equalIndex);
            if (!"fileId".equals(key)) continue;
            try {
                return parsePositiveLong(URLDecoder.decode(param.substring(equalIndex + 1), StandardCharsets.UTF_8));
            } catch (Exception e) {
                return null;
            }
        }
        return null;
    }

    @Override
    public ResponseResult<String> sendMail(Long mailBoxId, String sendName, List<String> toAddressList, List<String> ccAddressList, List<String> bccAddressList, String subject, String bodyData, List<SendMailAttachmentRequest> attachments, Long replySourceMappingId, String baseUrl) {
        if (mailBoxId == null || mailBoxId <= 0)  return ResponseResult.error(ErrorCode.PARAM_INVALID, "发件邮箱不能为空");
        if (toAddressList == null || toAddressList.isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "收件人不能为空");
        if (bodyData == null) bodyData = "";
        if (subject == null)  subject = "";
        if (sendName == null) sendName = "";

        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess() || currentUser.getData() == null) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        Users user = currentUser.getData();

        // 查询发件邮箱
        ResponseResult<MailBoxes> mailBoxResult = mailBoxesService.getMailBoxById(mailBoxId);
        if (!mailBoxResult.isSuccess() || mailBoxResult.getData() == null) return ResponseResult.error(ErrorCode.MAILBOX_NOT_FOUND, "发件邮箱不存在");
        MailBoxes fromMailBox = mailBoxResult.getData();

        // 校验当前用户是否拥有该邮箱
        if (!fromMailBox.getUserId().equals(user.getUserId())) return ResponseResult.error(ErrorCode.FORBIDDEN, "无权使用该发件邮箱");

        // 获取完整发件地址，例如 123456@lby.email
        ResponseResult<String> fromAddressResult = mailBoxesService.getMailBoxAddress(mailBoxId);
        if (!fromAddressResult.isSuccess() || fromAddressResult.getData() == null || fromAddressResult.getData().isBlank()) return ResponseResult.error(ErrorCode.MAILBOX_NOT_FOUND, "发件邮箱地址不存在");
        String fromAddress = fromAddressResult.getData();

        List<String> normalizedToAddresses;
        List<String> normalizedCcAddresses;
        List<String> normalizedBccAddresses;
        try {
            normalizedToAddresses = normalizeEmailAddresses(toAddressList);
            normalizedCcAddresses = normalizeEmailAddresses(ccAddressList);
            normalizedBccAddresses = normalizeEmailAddresses(bccAddressList);
        } catch (IllegalArgumentException e) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, e.getMessage());
        }
        if (normalizedToAddresses.isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "收件人不能为空");

        // 生成原始 MIME 邮件正文，C++ 端会把这个 body 当作邮件原文存储和投递
        byte[] rawMimeBytes;
        try {
            bodyData = replaceUserFileLinksWithSharedLinks(user, bodyData, baseUrl);
            rawMimeBytes = buildRawMimeBytes(fromAddress, sendName, normalizedToAddresses, normalizedCcAddresses, normalizedBccAddresses, subject, bodyData, attachments);
        } catch (Exception e) {
            logger.error("构建邮件 MIME 失败", e);
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "邮件内容构建失败");
        }

        if (rawMimeBytes.length == 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮件内容不能为空");

        // 组装 Hub metadata
        Map<String, Object> metadata = new HashMap<>();
        metadata.put("action", "RelayPolicySendMail");
        Map<String, Object> mailFrom = buildMailFromMetadata(fromMailBox, user, fromAddress, sendName, rawMimeBytes.length);
        metadata.put("mailFrom", mailFrom);

        // 构建收件人列表
        List<Map<String, Object>> rcptTos = new ArrayList<>();
        Set<String> allRecipientAddresses = new LinkedHashSet<>();
        allRecipientAddresses.addAll(normalizedToAddresses);
        allRecipientAddresses.addAll(normalizedCcAddresses);
        allRecipientAddresses.addAll(normalizedBccAddresses);
        for (String toAddress : allRecipientAddresses) {
            ResponseResult<MailBoxAddressToMailBoxAndUser> targetResult = mailBoxesService.getAccountByMailBoxAddress(toAddress);
            if (targetResult.isSuccess() && targetResult.getData() != null) {
                rcptTos.add(buildLocalRcptToMetadata(toAddress, targetResult.getData()));
                continue;
            }
            // 域名不存在，说明不是本系统托管域名，按外发处理
            if (targetResult.getCode() == ErrorCode.DOMAIN_NOT_FOUND) {
                rcptTos.add(buildOutboundRcptToMetadata(toAddress));
                continue;
            }
            // 域名存在但邮箱不存在，不能当外发， 拒绝
            return ResponseResult.error(targetResult.getCode(), "收件人无效：" + toAddress + "，" + targetResult.getMessage());
        }

        // 收件人验证
        if (rcptTos.isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "收件人不能为空");
        metadata.put("rcptTos", rcptTos);

        // 发送 Hub 请求
        RpcMessage request = RpcCodec.encode(RpcType.REQUEST, RequestIdGenerator.nextId(), RpcCode.NONE, metadata, rawMimeBytes, 30000, 1);
        try {
            RpcMessage response = novaMailHubClient.sendRequest(request).get(8, java.util.concurrent.TimeUnit.SECONDS);
            if (response.getCode() != RpcCode.OK) {
                logger.warn("RelayPolicySendMail rejected code={} metadata={}", response.getCode(), response.getMetadata());
                return ResponseResult.error(ErrorCode.HUB_ERROR, "邮件提交失败");
            }
            if (replySourceMappingId != null && replySourceMappingId > 0) setAnswered(user, replySourceMappingId, true);
            return ResponseResult.success("邮件已提交发送队列");
        } catch (Exception e) {
            logger.error("RelayPolicySendMail request failed", e);
            return ResponseResult.error(ErrorCode.HUB_ERROR, "邮件提交超时或Hub异常");
        }
    }

    @Override
    @Transactional
    public ResponseResult<SaveDraftResponse> saveDraft(Long draftMappingId, Long mailBoxId, String sendName, List<String> toAddressList, List<String> ccAddressList, List<String> bccAddressList, String subject, String bodyData, List<SendMailAttachmentRequest> attachments) {
        try {
            if (bodyData == null) bodyData = "";
            if (subject == null) subject = "";
            if (sendName == null) sendName = "";

            ResponseResult<Users> currentUser = usersService.getCurrentUser();
            if (!currentUser.isSuccess() || currentUser.getData() == null) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
            Users user = currentUser.getData();

            requireOwnedMailBox(user, mailBoxId);
            String fromAddress = requireMailBoxAddress(mailBoxId);
            List<String> normalizedToAddresses = normalizeDraftAddressList(toAddressList);
            List<String> normalizedCcAddresses = normalizeDraftAddressList(ccAddressList);
            List<String> normalizedBccAddresses = normalizeDraftAddressList(bccAddressList);

            byte[] rawMimeBytes = buildRawMimeBytes(fromAddress, sendName, normalizedToAddresses, normalizedCcAddresses, normalizedBccAddresses, subject, bodyData, attachments);
            if (rawMimeBytes.length == 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "草稿内容不能为空");

            DraftMappingInfo existingDraft = null;
            if (draftMappingId != null && draftMappingId > 0) {
                existingDraft = emailMapper.getDraftMappingForUpdate(user.getUserId(), draftMappingId, DRAFT_FOLDER_ID);
                if (existingDraft == null) return ResponseResult.error(ErrorCode.NOT_FOUND, "草稿不存在或无权编辑");
            }

            Long mailId = existingDraft == null ? nextMailId() : existingDraft.getMailId();
            String filePath = existingDraft == null ? buildMailPath(mailId) : existingDraft.getFilePath();
            if (filePath == null || filePath.isBlank()) filePath = buildMailPath(mailId);

            Email email = new Email();
            email.setMailId(mailId);
            email.setSenderName(trimToLength(sendName.isBlank() ? fromAddress : sendName, SENDER_NAME_MAX_LENGTH));
            email.setSenderEmail(fromAddress);
            email.setSubject(trimToLength(subject, SUBJECT_MAX_LENGTH));
            email.setPreview(htmlToPreview(bodyData));
            email.setFilePath(filePath);
            email.setStorageBucket(MAIL_STORAGE_BUCKET);

            if (existingDraft != null && !Objects.equals(existingDraft.getMailBoxId(), mailBoxId)) {
                filePath = buildMailPath(mailId);
                email.setFilePath(filePath);
            }

            if (existingDraft == null) {
                if (emailMapper.insertMail(email) <= 0) throw new IllegalStateException("草稿邮件记录创建失败");
            } else {
                if (emailMapper.updateMail(email) <= 0) throw new IllegalStateException("草稿邮件记录更新失败");
            }

            SaveDraftResponse response = new SaveDraftResponse();
            response.setMailId(mailId);
            response.setMailBoxId(mailBoxId);
            response.setFolderId(DRAFT_FOLDER_ID);
            if (existingDraft == null) {
                response.setImapUid(allocateImapUid(mailBoxId, DRAFT_FOLDER_ID));
                if (emailMapper.insertDraftMapping(response) <= 0 || response.getMappingId() == null || response.getMappingId() <= 0) {
                    throw new IllegalStateException("草稿邮箱映射创建失败");
                }
            } else {
                response.setMappingId(existingDraft.getMappingId());
                if (Objects.equals(existingDraft.getMailBoxId(), mailBoxId)) {
                    response.setImapUid(existingDraft.getImapUid());
                } else {
                    response.setImapUid(allocateImapUid(mailBoxId, DRAFT_FOLDER_ID));
                }
                if (emailMapper.updateDraftMapping(response) <= 0) throw new IllegalStateException("草稿邮箱映射更新失败");
            }

            MinIoBucket minIoBucket = minIoFactory.openBucket(MAIL_STORAGE_BUCKET, true);
            try (InputStream inputStream = new ByteArrayInputStream(rawMimeBytes)) {
                minIoBucket.uploadFile(filePath, inputStream, "message/rfc822", rawMimeBytes.length);
            }

            return ResponseResult.success("草稿已保存", response);
        } catch (SecurityException e) {
            TransactionAspectSupport.currentTransactionStatus().setRollbackOnly();
            return ResponseResult.error(ErrorCode.FORBIDDEN, e.getMessage());
        } catch (IllegalArgumentException e) {
            TransactionAspectSupport.currentTransactionStatus().setRollbackOnly();
            return ResponseResult.error(ErrorCode.PARAM_INVALID, e.getMessage());
        } catch (IllegalStateException e) {
            TransactionAspectSupport.currentTransactionStatus().setRollbackOnly();
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, e.getMessage());
        } catch (Exception e) {
            TransactionAspectSupport.currentTransactionStatus().setRollbackOnly();
            logger.error("保存草稿失败", e);
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "草稿保存失败");
        }
    }

    @Override
    public ResponseResult<DraftDetailResponse> getDraft(Users user, Long mappingId) {
        if (user == null || user.getUserId() == null) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (mappingId == null || mappingId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "草稿ID不能为空");

        DraftMappingInfo draft = emailMapper.getDraftMapping(user.getUserId(), mappingId, DRAFT_FOLDER_ID);
        if (draft == null) return ResponseResult.error(ErrorCode.NOT_FOUND, "草稿不存在或无权访问");

        try (InputStream inputStream = downloadMail(draft.getStorageBucket(), draft.getFilePath())) {
            Session session = Session.getDefaultInstance(new Properties());
            MimeMessage message = new MimeMessage(session, inputStream);

            DraftDetailResponse response = new DraftDetailResponse();
            response.setMappingId(draft.getMappingId());
            response.setMailId(draft.getMailId());
            response.setMailBoxId(draft.getMailBoxId());
            response.setSendName(parsePersonal(message.getFrom(), ""));
            ResponseResult<String> mailBoxAddress = mailBoxesService.getMailBoxAddress(draft.getMailBoxId());
            response.setSendMailBoxView(mailBoxAddress.isSuccess() ? mailBoxAddress.getData() : "");
            response.setSubject(safeText(message.getSubject()));
            response.setBodyData(extractDraftBody(message));
            response.setToAddressList(parseAddressStrings(message.getRecipients(Message.RecipientType.TO)));
            response.setCcAddressList(parseAddressStrings(message.getRecipients(Message.RecipientType.CC)));
            response.setBccAddressList(parseAddressStrings(message.getRecipients(Message.RecipientType.BCC)));
            response.setAttachments(extractDraftAttachments(message));
            return ResponseResult.success(response);
        } catch (Exception e) {
            logger.error("读取草稿失败 mappingId={}", mappingId, e);
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "草稿读取失败");
        }
    }

    @Override
    public List<RestMailFolder> getMailFolder(Users user) {
        List<RestMailFolder> folders = emailMapper.getMailFolder(user.getUserId());

        RestMailFolder starred = new RestMailFolder();
        starred.setFolderId(-1L);
        starred.setFolderName("STAR");
        starred.setNoReadCont(emailMapper.getStarredNoReadCont(user.getUserId()));

        folders.add(starred);
        return folders;
    }

    @Override
    public List<MailBoxMailMapping> getEmailList(Users user, RestMailFolder mailFolder) {
        if (mailFolder.getFolderId() == -1) return emailMapper.getStarredMailMappings(user.getUserId());
        return emailMapper.getMailBoxMailMappings(user.getUserId(), mailFolder.getFolderId());
    }

    @Override
    public MailDetailResponse getMailDetail(Users user, Long mappingId) {
        if (user == null || user.getUserId() == null) throw new RuntimeException("用户未登录");
        if (mappingId == null) throw new RuntimeException("mappingId 不能为空");

        MailDetailStorageInfo storageInfo = emailMapper.getMailDetailStorageInfo(user.getUserId(), mappingId);

        if (storageInfo == null) throw new RuntimeException("邮件不存在或无权访问");

        ParsedMailBody body = new ParsedMailBody();

        try (InputStream inputStream = downloadMail(storageInfo.getStorageBucket(), storageInfo.getFilePath())) {
            body = mailMimeParseService.parse(inputStream);
        } catch (Exception e) {
            logger.warn("邮件正文解析失败，返回基础详情。mappingId={}, mailId={}", mappingId, storageInfo.getMailId(), e);
            body.setTextBody(storageInfo.getPreview());
        }

        MailDetailResponse response = getMailDetailResponse(storageInfo, body);

        // 设为已读
        setMailReader(user, mappingId, true);

        List<MailAttachmentResponse> attachments = mergeAttachmentResponses(mappingId, emailMapper.getMailAttachments(storageInfo.getMailId()), body.getAttachments());
        response.setAttachments(attachments);

        return response;
    }

    private List<MailAttachmentResponse> mergeAttachmentResponses(Long mappingId, List<MailAttachmentResponse> databaseAttachments, List<MailAttachmentResponse> parsedAttachments) {
        List<MailAttachmentResponse> merged = new ArrayList<>();

        if (databaseAttachments != null) {
            for (MailAttachmentResponse attachment : databaseAttachments) {
                if (attachment == null) continue;
                attachment.setDownloadUrl("/email/downloadAttachment");
                attachment.setSourceType("DATABASE");
                attachment.setMappingId(mappingId);
                merged.add(attachment);
            }
        }

        if (parsedAttachments == null || parsedAttachments.isEmpty()) return merged;

        for (MailAttachmentResponse parsedAttachment : parsedAttachments) {
            if (parsedAttachment == null || !Boolean.FALSE.equals(parsedAttachment.getIsInline())) continue;
            if (merged.stream().anyMatch(existing -> sameAttachment(existing, parsedAttachment))) continue;

            parsedAttachment.setDownloadUrl("/email/downloadParsedAttachment");
            parsedAttachment.setSourceType("MIME_PART");
            parsedAttachment.setMappingId(mappingId);
            merged.add(parsedAttachment);
        }

        return merged;
    }

    private boolean sameAttachment(MailAttachmentResponse left, MailAttachmentResponse right) {
        if (left == null || right == null) return false;
        String leftName = normalizeAttachmentCompareValue(left.getFileName());
        String rightName = normalizeAttachmentCompareValue(right.getFileName());
        if (!leftName.equals(rightName)) return false;

        Long leftSize = left.getFileSize();
        Long rightSize = right.getFileSize();
        if (leftSize != null && rightSize != null && leftSize > 0 && rightSize > 0) return leftSize.equals(rightSize);

        String leftType = normalizeAttachmentCompareValue(left.getFileType());
        String rightType = normalizeAttachmentCompareValue(right.getFileType());
        return leftType.equals(rightType);
    }

    private String normalizeAttachmentCompareValue(String value) {
        return value == null ? "" : value.trim().toLowerCase();
    }

    @Override
    public boolean setMailReader(Users user, Long mappingId, boolean reader) {
        boolean updated = emailMapper.setMailReader(user.getUserId(), mappingId, reader) > 0;
        if (updated) mailNotifyService.notifyMailStatusChanged(user.getAccount(), mappingId, "Reader");
        return updated;
    }

    @Override
    public boolean setStarred(Users user, Long mappingId, boolean starred) {
        boolean updated = emailMapper.setStarred(user.getUserId(), mappingId, starred) > 0;
        if (updated) mailNotifyService.notifyMailStatusChanged(user.getAccount(), mappingId, "Starred");
        return updated;
    }

    @Override
    public boolean setAnswered(Users user, Long mappingId, boolean answered) {
        boolean updated = emailMapper.setAnswered(user.getUserId(), mappingId, answered) > 0;
        if (updated) mailNotifyService.notifyMailStatusChanged(user.getAccount(), mappingId, "Answered");
        return updated;
    }

    @Override
    @Transactional
    public ResponseResult<Void> moveMail(Users user, Long mappingId, Long targetFolderId) {
        try {
            if (user == null || user.getUserId() == null) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "login required");
            if (mappingId == null || mappingId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "invalid mappingId");
            if (targetFolderId == null || targetFolderId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "invalid target folder");
            if (emailMapper.countAccessibleFolder(user.getUserId(), targetFolderId) <= 0) return ResponseResult.error(ErrorCode.NOT_FOUND, "target folder not found");

            DraftMappingInfo mapping = emailMapper.getMailMappingForUpdate(user.getUserId(), mappingId);
            if (mapping == null) return ResponseResult.error(ErrorCode.NOT_FOUND, "mail not found or forbidden");
            if (Objects.equals(mapping.getFolderId(), targetFolderId)) return ResponseResult.success();

            Long nextUid = allocateImapUid(mapping.getMailBoxId(), targetFolderId);
            if (emailMapper.moveMailMapping(mappingId, targetFolderId, nextUid) <= 0) {
                TransactionAspectSupport.currentTransactionStatus().setRollbackOnly();
                return ResponseResult.error(ErrorCode.DATABASE_ERROR, "move mail failed");
            }

            mailNotifyService.notifyMailMoved(user.getAccount(), mappingId, mapping.getFolderId(), targetFolderId);
            return ResponseResult.success();
        } catch (Exception e) {
            TransactionAspectSupport.currentTransactionStatus().setRollbackOnly();
            logger.error("move mail failed mappingId={}, targetFolderId={}", mappingId, targetFolderId, e);
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "move mail failed");
        }
    }

    @Override
    @Transactional
    public ResponseResult<Void> deleteMail(Users user, Long mappingId) {
        try {
            if (user == null || user.getUserId() == null) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "login required");
            if (mappingId == null || mappingId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "invalid mappingId");

            DraftMappingInfo mapping = emailMapper.getMailMappingForUpdate(user.getUserId(), mappingId);
            if (mapping == null) return ResponseResult.error(ErrorCode.NOT_FOUND, "mail not found or forbidden");

            Long targetFolderId = Objects.equals(mapping.getFolderId(), RECYCLE_FOLDER_ID) || Objects.equals(mapping.getFolderId(), DELETED_FOLDER_ID)
                    ? DELETED_FOLDER_ID
                    : RECYCLE_FOLDER_ID;
            if (Objects.equals(mapping.getFolderId(), targetFolderId)) return ResponseResult.success();

            Long nextUid = allocateImapUid(mapping.getMailBoxId(), targetFolderId);
            if (emailMapper.moveMailMapping(mappingId, targetFolderId, nextUid) <= 0) {
                TransactionAspectSupport.currentTransactionStatus().setRollbackOnly();
                return ResponseResult.error(ErrorCode.DATABASE_ERROR, "delete mail failed");
            }

            mailNotifyService.notifyMailMoved(user.getAccount(), mappingId, mapping.getFolderId(), targetFolderId);
            return ResponseResult.success();
        } catch (Exception e) {
            TransactionAspectSupport.currentTransactionStatus().setRollbackOnly();
            logger.error("delete mail failed mappingId={}", mappingId, e);
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "delete mail failed");
        }
    }

    @Override
    public List<SearchSuggestionVO> getSearchSuggestions(Users user, String keyword, Integer limit) {
        if (user == null || user.getUserId() == null) return List.of();
        if (keyword == null || keyword.trim().isEmpty()) return List.of();

        String searchKeyword = keyword.trim();

        // 限制提示数量，避免前端频繁输入时查询压力过大
        int safeLimit = limit == null ? 8 : Math.max(1, Math.min(limit, 20));

        // 防止 LIKE 查询中 %、_ 被当成通配符
        String likeKeyword = escapeLikeKeyword(searchKeyword);
        return emailMapper.searchSuggestions(user.getUserId(), likeKeyword, safeLimit);
    }

    // 转义 LIKE 关键字
    private String escapeLikeKeyword(String keyword) {
        return keyword.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_");
    }

    @Override
    public void downloadAttachment(Users user, DownloadAttachmentRequest request, HttpServletResponse response) throws Exception {
        streamAttachment(user, request, response, false);
    }

    @Override
    public void previewAttachment(Users user, DownloadAttachmentRequest request, HttpServletResponse response) throws Exception {
        streamAttachment(user, request, response, true);
    }

    private void streamAttachment(Users user, DownloadAttachmentRequest request, HttpServletResponse response, boolean inline) throws Exception {
        if (user == null || user.getUserId() == null) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        Long attachmentId = request == null ? null : request.getAttachmentId();
        if (attachmentId == null || attachmentId <= 0) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }

        MailAttachmentDownloadInfo info = emailMapper.getAttachmentDownloadInfo(user.getUserId(), attachmentId);
        if (info == null || info.getFilePath() == null || info.getFilePath().isBlank()) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }

        Long userFileId = parseUserFileReference(info.getFilePath());
        if (userFileId != null) {
            userFilesService.streamReferencedFile(userFileId, response, inline);
            return;
        }

        String bucketName = info.getStorageBucket();
        String objectName = info.getFilePath();
        if (bucketName == null || bucketName.isBlank() || objectName == null || objectName.isBlank()) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }

        MinIoBucket minIoBucket = minIoFactory.openBucket(bucketName);
        if (!minIoBucket.fileExists(objectName)) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }

        String contentType = info.getFileType();
        if (contentType == null || contentType.isBlank()) contentType = minIoBucket.getFileType(objectName);
        if (contentType == null || contentType.isBlank()) contentType = "application/octet-stream";

        response.setContentType(contentType);
        if (info.getFileSize() != null) response.setHeader("Content-Length", String.valueOf(info.getFileSize()));
        response.setHeader("Content-Disposition", buildAttachmentDisposition(info.getFileName(), inline));
        response.setHeader("X-Content-Type-Options", "nosniff");

        try (InputStream inputStream = minIoBucket.downloadFile(objectName)) {
            inputStream.transferTo(response.getOutputStream());
        }
    }

    @Override
    public void downloadParsedAttachment(Users user, DownloadParsedAttachmentRequest request, HttpServletResponse response) throws Exception {
        streamParsedAttachment(user, request, response, false);
    }

    @Override
    public void previewParsedAttachment(Users user, DownloadParsedAttachmentRequest request, HttpServletResponse response) throws Exception {
        streamParsedAttachment(user, request, response, true);
    }

    private void streamParsedAttachment(Users user, DownloadParsedAttachmentRequest request, HttpServletResponse response, boolean inline) throws Exception {
        if (user == null || user.getUserId() == null) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        Long mappingId = request == null ? null : request.getMappingId();
        Integer partIndex = request == null ? null : request.getPartIndex();
        if (mappingId == null || mappingId <= 0 || partIndex == null || partIndex < 0) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }

        MailDetailStorageInfo storageInfo = emailMapper.getMailDetailStorageInfo(user.getUserId(), mappingId);
        if (storageInfo == null) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }

        try (InputStream inputStream = downloadMail(storageInfo.getStorageBucket(), storageInfo.getFilePath())) {
            Session session = Session.getDefaultInstance(new Properties());
            MimeMessage message = new MimeMessage(session, inputStream);
            BodyPart attachmentPart = findAttachmentPart(message, partIndex, new AtomicInteger(0));
            if (attachmentPart == null) {
                response.setStatus(HttpServletResponse.SC_NOT_FOUND);
                return;
            }

            String fileName = decodeMimeFileName(attachmentPart.getFileName(), "attachment-" + (partIndex + 1));
            String contentType = normalizeParsedAttachmentContentType(attachmentPart.getContentType());
            response.setContentType(contentType);
            response.setHeader("Content-Disposition", buildAttachmentDisposition(fileName, inline));
            response.setHeader("X-Content-Type-Options", "nosniff");

            ByteArrayOutputStream buffer = new ByteArrayOutputStream();
            try (InputStream partInputStream = attachmentPart.getInputStream()) {
                partInputStream.transferTo(buffer);
            }
            response.setHeader("Content-Length", String.valueOf(buffer.size()));
            buffer.writeTo(response.getOutputStream());
        }
    }

    private BodyPart findAttachmentPart(Part part, int targetPartIndex, AtomicInteger partCounter) throws Exception {
        Object content = part.getContent();
        if (!(content instanceof MimeMultipart multipart)) return null;

        for (int i = 0; i < multipart.getCount(); i++) {
            BodyPart bodyPart = multipart.getBodyPart(i);
            int currentPartIndex = partCounter.getAndIncrement();
            if (currentPartIndex == targetPartIndex && isMimeAttachmentPart(bodyPart)) return bodyPart;
            if (bodyPart.isMimeType("multipart/*")) {
                BodyPart found = findAttachmentPart(bodyPart, targetPartIndex, partCounter);
                if (found != null) return found;
            }
        }
        return null;
    }

    private boolean isMimeAttachmentPart(Part part) throws Exception {
        String disposition = part.getDisposition();
        if (Part.ATTACHMENT.equalsIgnoreCase(disposition)) return true;
        String fileName = part.getFileName();
        return fileName != null && !fileName.isBlank() && !Part.INLINE.equalsIgnoreCase(disposition);
    }

    private String decodeMimeFileName(String fileName, String fallback) {
        if (fileName == null || fileName.isBlank()) return fallback;
        try {
            fileName = MimeUtility.decodeText(fileName);
        } catch (Exception ignored) {
        }
        fileName = fileName.replace("\\", "/");
        int slashIndex = fileName.lastIndexOf('/');
        if (slashIndex >= 0) fileName = fileName.substring(slashIndex + 1);
        fileName = fileName.trim();
        return fileName.isBlank() ? fallback : fileName;
    }

    private String normalizeParsedAttachmentContentType(String contentType) {
        if (contentType == null || contentType.isBlank()) return "application/octet-stream";
        int separatorIndex = contentType.indexOf(';');
        if (separatorIndex >= 0) contentType = contentType.substring(0, separatorIndex);
        contentType = contentType.trim();
        return contentType.isBlank() ? "application/octet-stream" : contentType;
    }

    private Long parseUserFileReference(String fileUrl) {
        if (fileUrl == null) return null;
        String value = fileUrl.trim();
        if (value.isBlank()) return null;

        if (value.startsWith("user-file://")) return parsePositiveLong(value.substring("user-file://".length()));
        if (value.startsWith("user-file:")) return parsePositiveLong(value.substring("user-file:".length()));

        int fileIdIndex = value.indexOf("fileId=");
        if (fileIdIndex < 0) return null;
        String idPart = value.substring(fileIdIndex + "fileId=".length());
        int ampIndex = idPart.indexOf('&');
        if (ampIndex >= 0) idPart = idPart.substring(0, ampIndex);
        return parsePositiveLong(idPart);
    }

    private Long parsePositiveLong(String value) {
        try {
            long parsed = Long.parseLong(value.trim());
            return parsed > 0 ? parsed : null;
        } catch (Exception e) {
            return null;
        }
    }

    private String buildAttachmentDisposition(String fileName, boolean inline) {
        String safeName = fileName == null || fileName.isBlank() ? "attachment" : fileName;
        String fallbackName = safeName.replace("\\", "_").replace("\"", "'");
        fallbackName = fallbackName.chars()
                .mapToObj(ch -> ch >= 0x20 && ch <= 0x7E ? String.valueOf((char) ch) : "_")
                .reduce("", String::concat);
        if (fallbackName.isBlank()) fallbackName = "attachment";
        String encodedName = URLEncoder.encode(safeName, StandardCharsets.UTF_8).replace("+", "%20");
        return (inline ? "inline" : "attachment") + "; filename=\"" + fallbackName + "\"; filename*=UTF-8''" + encodedName;
    }

    private String parsePersonal(Address[] addresses, String fallback) {
        if (addresses == null || addresses.length == 0) return fallback;
        Address first = addresses[0];
        if (first instanceof InternetAddress internetAddress) {
            String personal = internetAddress.getPersonal();
            if (personal != null && !personal.isBlank()) return personal;
            String address = internetAddress.getAddress();
            return address == null || address.isBlank() ? fallback : address;
        }
        return first == null ? fallback : first.toString();
    }

    private List<String> parseAddressStrings(Address[] addresses) {
        List<String> result = new ArrayList<>();
        if (addresses == null) return result;
        for (Address address : addresses) {
            if (address instanceof InternetAddress internetAddress) {
                String value = internetAddress.getAddress();
                if (value != null && !value.isBlank()) result.add(value);
            } else if (address != null && !address.toString().isBlank()) {
                result.add(address.toString());
            }
        }
        return result;
    }

    private String extractDraftBody(Part part) throws Exception {
        Object content = part.getContent();
        if (content instanceof String text) return text;
        if (content instanceof MimeMultipart multipart) {
            String plainText = "";
            for (int i = 0; i < multipart.getCount(); i++) {
                BodyPart bodyPart = multipart.getBodyPart(i);
                if (isMimeAttachmentPart(bodyPart)) continue;
                if (bodyPart.isMimeType("text/html")) {
                    Object childContent = bodyPart.getContent();
                    if (childContent instanceof String html) return html;
                }
                if (bodyPart.isMimeType("text/plain") && plainText.isBlank()) {
                    Object childContent = bodyPart.getContent();
                    if (childContent instanceof String text) plainText = text;
                }
                if (bodyPart.isMimeType("multipart/*")) {
                    String nestedBody = extractDraftBody(bodyPart);
                    if (!nestedBody.isBlank()) return nestedBody;
                }
            }
            return plainText;
        }
        return "";
    }

    private List<SendMailAttachmentRequest> extractDraftAttachments(Part part) throws Exception {
        List<SendMailAttachmentRequest> attachments = new ArrayList<>();
        collectDraftAttachments(part, attachments);
        return attachments;
    }

    private void collectDraftAttachments(Part part, List<SendMailAttachmentRequest> attachments) throws Exception {
        if (isMimeAttachmentPart(part)) {
            attachments.add(toDraftAttachment(part));
            return;
        }
        Object content = part.getContent();
        if (!(content instanceof MimeMultipart multipart)) return;
        for (int i = 0; i < multipart.getCount(); i++) collectDraftAttachments(multipart.getBodyPart(i), attachments);
    }

    private SendMailAttachmentRequest toDraftAttachment(Part part) throws Exception {
        SendMailAttachmentRequest attachment = new SendMailAttachmentRequest();
        String fileName = decodeMimeFileName(part.getFileName(), "attachment");
        String fileType = normalizeParsedAttachmentContentType(part.getContentType());
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        try (InputStream inputStream = part.getInputStream()) {
            inputStream.transferTo(buffer);
        }
        attachment.setFileName(fileName);
        attachment.setFileType(fileType);
        attachment.setFileSize((long) buffer.size());
        attachment.setFileData("data:" + fileType + ";base64," + Base64.getEncoder().encodeToString(buffer.toByteArray()));
        attachment.setSendType("MIME_ATTACHMENT");
        return attachment;
    }

    @NotNull
    private static MailDetailResponse getMailDetailResponse(MailDetailStorageInfo storageInfo, ParsedMailBody body) {
        MailDetailResponse response = new MailDetailResponse();
        response.setMappingId(storageInfo.getMappingId());
        response.setMailId(storageInfo.getMailId());
        response.setFolderId(storageInfo.getFolderId());

        MailAddressInfo mailAddressInfo = new MailAddressInfo();
        mailAddressInfo.setName(storageInfo.getSenderName());
        mailAddressInfo.setAddress(storageInfo.getSenderEmail());
        response.setFrom(mailAddressInfo);
        response.setTos(body.getTo());
        response.setCcs(body.getCc());

        response.setSubject(storageInfo.getSubject());
        response.setPreview(storageInfo.getPreview());
        response.setCreateTime(storageInfo.getCreateTime());

        response.setIsRead(storageInfo.getIsRead());
        response.setIsStarred(storageInfo.getIsStarred());
        response.setIsAnswered(storageInfo.getIsAnswered());

        response.setHtmlBody(body.getHtmlBody());
        response.setTextBody(body.getTextBody());
        return response;
    }

    private InputStream downloadMail(String bucketName, String objectName) throws Exception {
        if (bucketName == null || bucketName.isBlank()) throw new IllegalArgumentException("bucketName 不能为空");
        if (objectName == null || objectName.isBlank()) throw new IllegalArgumentException("objectName 不能为空");

        MinIoBucket minIoBucket = minIoFactory.openBucket(bucketName);

        if (!minIoBucket.fileExists(objectName)) throw new FileNotFoundException("邮件文件不存在: " + objectName);
        return minIoBucket.downloadFile(objectName);
    }
}
