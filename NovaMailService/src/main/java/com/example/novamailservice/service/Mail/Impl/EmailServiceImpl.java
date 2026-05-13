package com.example.novamailservice.service.Mail.Impl;

import com.example.novamailservice.dto.Mail.*;
import com.example.novamailservice.dto.MailBoxAddressToMailBoxAndUser;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Mail.MailBoxes;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.mapper.Mail.EmailMapper;
import com.example.novamailservice.service.Mail.EmailService;
import com.example.novamailservice.service.Mail.MailBoxesService;
import com.example.novamailservice.service.Mail.MailMimeParseService;
import com.example.novamailservice.service.Mail.MailNotifyService;
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

import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import jakarta.mail.Message;
import jakarta.mail.Session;
import jakarta.mail.internet.InternetAddress;
import jakarta.mail.internet.MimeMessage;
import java.io.ByteArrayOutputStream;
import java.util.Properties;

@Service
public class EmailServiceImpl implements EmailService {
    private final Logger logger = LoggerFactory.getLogger(EmailServiceImpl.class.getName());
    private final EmailMapper emailMapper;
    private final UsersService usersService;
    private final NovaMailHubClient novaMailHubClient;
    private final MailNotifyService mailNotifyService;
    private final MailBoxesService mailBoxesService;
    private final MailMimeParseService mailMimeParseService;
    private final MinIoFactory minIoFactory;

    @Autowired
    public EmailServiceImpl(EmailMapper emailMapper, UsersService usersService, NovaMailHubClient novaMailHubClient, MailNotifyService mailNotifyService, MailBoxesService mailBoxesService, MailMimeParseService mailMimeParseService, MinIoFactory minIoFactory) {
        this.emailMapper = emailMapper;
        this.usersService = usersService;
        this.novaMailHubClient = novaMailHubClient;
        this.mailNotifyService = mailNotifyService;
        this.mailBoxesService = mailBoxesService;
        this.mailMimeParseService = mailMimeParseService;
        this.minIoFactory = minIoFactory;
    }

    private byte[] buildRawMimeBytes(String fromAddress, String sendName, List<String> toAddresses, String subject, String htmlBody) throws Exception {
        Properties props = new Properties();
        Session session = Session.getInstance(props);

        MimeMessage message = new MimeMessage(session);
        InternetAddress from = new InternetAddress(fromAddress, sendName == null ? "" : sendName, java.nio.charset.StandardCharsets.UTF_8.name());
        message.setFrom(from);
        for (String to : toAddresses) message.addRecipient(Message.RecipientType.TO, new InternetAddress(to));

        message.setSubject(subject == null ? "（无题）" : subject, java.nio.charset.StandardCharsets.UTF_8.name());
        message.setContent(htmlBody == null ? "" : htmlBody, "text/html;charset=UTF-8");
        message.setSentDate(new java.util.Date());
        message.saveChanges();

        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        message.writeTo(outputStream);
        return outputStream.toByteArray();
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

    @Override
    public ResponseResult<String> sendMail(Long mailBoxId, String sendName, List<String> toAddressList, String subject, String bodyData) {
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

        // 校验并规范化收件人地址
        List<String> normalizedToAddresses = new ArrayList<>();

        for (String rawTo : toAddressList) {
            String toAddress = normalizeEmailAddress(rawTo);
            if (toAddress == null || toAddress.isBlank()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "收件人地址格式错误：" + rawTo);
            normalizedToAddresses.add(toAddress);
        }

        // 生成原始 MIME 邮件正文，C++ 端会把这个 body 当作邮件原文存储和投递
        byte[] rawMimeBytes;
        try {
            rawMimeBytes = buildRawMimeBytes(fromAddress, sendName, normalizedToAddresses, subject, bodyData);
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
        for (String toAddress : normalizedToAddresses) {
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
            return ResponseResult.success("邮件已提交发送队列");
        } catch (Exception e) {
            logger.error("RelayPolicySendMail request failed", e);
            return ResponseResult.error(ErrorCode.HUB_ERROR, "邮件提交超时或Hub异常");
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

        ParsedMailBody body;

        try (InputStream inputStream = downloadMail(storageInfo.getStorageBucket(), storageInfo.getFilePath())) {
            body = mailMimeParseService.parse(inputStream);
        } catch (Exception e) {
            throw new RuntimeException("邮件内容读取失败", e);
        }

        MailDetailResponse response = getMailDetailResponse(storageInfo, body);

        // 设为已读
        setMailReader(user, mappingId, true);

        // 发送通知
        mailNotifyService.notifyMailStatusChanged(user.getAccount(), mappingId, "Reader");

        List<MailAttachmentResponse> attachments = emailMapper.getMailAttachments(storageInfo.getMailId());
        for (MailAttachmentResponse attachment : attachments) {
            attachment.setDownloadUrl("/email/downloadAttachment?attachmentId=" + attachment.getAttachmentId());
        }

        return response;
    }

    @Override
    public boolean setMailReader(Users user, Long mappingId, boolean reader) {
        return emailMapper.setMailReader(user.getUserId(), mappingId, reader) > 0;
    }

    @Override
    public boolean setStarred(Users user, Long mappingId, boolean starred) {
        return emailMapper.setStarred(user.getUserId(), mappingId, starred) > 0;
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

    @NotNull
    private static MailDetailResponse getMailDetailResponse(MailDetailStorageInfo storageInfo, ParsedMailBody body) {
        MailDetailResponse response = new MailDetailResponse();
        response.setMappingId(storageInfo.getMappingId());
        response.setMailId(storageInfo.getMailId());

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
