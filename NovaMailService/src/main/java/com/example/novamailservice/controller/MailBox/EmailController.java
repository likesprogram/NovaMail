package com.example.novamailservice.controller.MailBox;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.example.novamailservice.dto.Mail.*;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.Mail.EmailService;
import com.example.novamailservice.service.User.UsersService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;
import io.swagger.v3.oas.annotations.Operation;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;

import java.io.IOException;
import java.util.Collections;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/email")
public class EmailController {
    private static final String DOWNLOAD_BODY_PARAMS_ATTRIBUTE = "DOWNLOAD_BODY_PARAMS";
    private final UsersService usersService;
    private final EmailService emailService;
    private final ObjectMapper objectMapper;

    @Autowired
    public EmailController(UsersService usersService, EmailService emailService, ObjectMapper objectMapper) {
        this.usersService = usersService;
        this.emailService = emailService;
        this.objectMapper = objectMapper;
    }

    @PostMapping("/sendMail")
    @Operation(summary = "发送邮件")
    public ResponseResult<String> sendMail(@RequestBody SendMailRequest request, HttpServletRequest servletRequest) {
        if (request == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "请求参数不能为空");
        return emailService.sendMail(
                request.getMailBoxId(),
                request.getSendName(),
                request.getToAddressList(),
                request.getCcAddressList(),
                request.getBccAddressList(),
                request.getSubject(),
                request.getBodyData(),
                request.getAttachments(),
                request.getReplySourceMappingId(),
                buildBaseUrl(servletRequest)
        );
    }

    @PostMapping("/saveDraft")
    @Operation(summary = "保存草稿")
    public ResponseResult<SaveDraftResponse> saveDraft(@RequestBody SaveDraftRequest request) {
        if (request == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "请求参数不能为空");
        return emailService.saveDraft(
                request.getDraftMappingId(),
                request.getMailBoxId(),
                request.getSendName(),
                request.getToAddressList(),
                request.getCcAddressList(),
                request.getBccAddressList(),
                request.getSubject(),
                request.getBodyData(),
                request.getAttachments()
        );
    }

    @PostMapping("/getDraft")
    @Operation(summary = "获取草稿")
    public ResponseResult<DraftDetailResponse> getDraft(@RequestBody Map<String, Long> request) {
        if (request == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        Long mappingId = request.get("mappingId");
        if (mappingId == null || mappingId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return emailService.getDraft(currentUser.getData(), mappingId);
    }

    @PostMapping("/getFolderList")
    public ResponseResult<List<RestMailFolder>> getFolderList() {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return ResponseResult.success(emailService.getMailFolder(currentUser.getData()));
    }

    @PostMapping("/getEmailList")
    public ResponseResult<List<MailBoxMailMapping>> getEmailList(@RequestBody RestMailFolder restMailFolder) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return ResponseResult.success(emailService.getEmailList(currentUser.getData(), restMailFolder));
    }

    @PostMapping("/getMailDetail")
    public ResponseResult<MailDetailResponse> getMailDetail(@RequestBody Map<String, Long> request) {
        if (request == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        Long mappingId = request.get("mappingId");
        if (mappingId == null || mappingId == 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        MailDetailResponse detail = emailService.getMailDetail(currentUser.getData(), mappingId);
        return ResponseResult.success(detail);
    }

    @PostMapping("/downloadAttachment")
    @Operation(summary = "下载邮件附件")
    public void downloadAttachment(HttpServletRequest servletRequest, HttpServletResponse response) throws Exception {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        DownloadAttachmentRequest request = new DownloadAttachmentRequest();
        request.setAttachmentId(readLongParam(servletRequest, "attachmentId"));
        emailService.downloadAttachment(currentUser.getData(), request, response);
    }

    @PostMapping("/previewAttachment")
    @Operation(summary = "预览邮件附件")
    public void previewAttachment(HttpServletRequest servletRequest, HttpServletResponse response) throws Exception {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        DownloadAttachmentRequest request = new DownloadAttachmentRequest();
        request.setAttachmentId(readLongParam(servletRequest, "attachmentId"));
        emailService.previewAttachment(currentUser.getData(), request, response);
    }

    @PostMapping("/downloadParsedAttachment")
    @Operation(summary = "下载邮件原文解析附件")
    public void downloadParsedAttachment(HttpServletRequest servletRequest, HttpServletResponse response) throws Exception {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        DownloadParsedAttachmentRequest request = new DownloadParsedAttachmentRequest();
        request.setMappingId(readLongParam(servletRequest, "mappingId"));
        request.setPartIndex(readIntegerParam(servletRequest, "partIndex"));
        emailService.downloadParsedAttachment(currentUser.getData(), request, response);
    }

    @PostMapping("/previewParsedAttachment")
    @Operation(summary = "预览邮件原文解析附件")
    public void previewParsedAttachment(HttpServletRequest servletRequest, HttpServletResponse response) throws Exception {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        DownloadParsedAttachmentRequest request = new DownloadParsedAttachmentRequest();
        request.setMappingId(readLongParam(servletRequest, "mappingId"));
        request.setPartIndex(readIntegerParam(servletRequest, "partIndex"));
        emailService.previewParsedAttachment(currentUser.getData(), request, response);
    }

    @PostMapping("/setRead")
    public ResponseResult<Void> setRead(@RequestBody UpdateStatus updateStatus) {
        if (updateStatus == null || updateStatus.getMappingId() == null || updateStatus.getMappingId() <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "invalid mappingId");
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return emailService.setMailReader(currentUser.getData(), updateStatus.getMappingId(), updateStatus.isData())
                ? ResponseResult.success()
                : ResponseResult.error(ErrorCode.NOT_FOUND, "mail not found");
    }

    @PostMapping("/setStarred")
    public ResponseResult<Void> setStarred(@RequestBody UpdateStatus updateStatus) {
        if (updateStatus == null || updateStatus.getMappingId() == null || updateStatus.getMappingId() <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "invalid mappingId");
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return emailService.setStarred(currentUser.getData(), updateStatus.getMappingId(), updateStatus.isData())
                ? ResponseResult.success()
                : ResponseResult.error(ErrorCode.NOT_FOUND, "mail not found");
    }

    @PostMapping("/setAnswered")
    public ResponseResult<Void> setAnswered(@RequestBody UpdateStatus updateStatus) {
        if (updateStatus == null || updateStatus.getMappingId() == null || updateStatus.getMappingId() <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "invalid mappingId");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return emailService.setAnswered(currentUser.getData(), updateStatus.getMappingId(), updateStatus.isData())
                ? ResponseResult.success()
                : ResponseResult.error(ErrorCode.NOT_FOUND, "mail not found");
    }

    @PostMapping("/move")
    public ResponseResult<Void> move(@RequestBody MailMoveRequest request) {
        if (request == null || request.getMappingId() == null || request.getTargetFolderId() == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "invalid move request");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return emailService.moveMail(currentUser.getData(), request.getMappingId(), request.getTargetFolderId());
    }

    @PostMapping("/delete")
    public ResponseResult<Void> delete(@RequestBody UpdateStatus updateStatus) {
        if (updateStatus == null || updateStatus.getMappingId() == null || updateStatus.getMappingId() <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "invalid mappingId");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return emailService.deleteMail(currentUser.getData(), updateStatus.getMappingId());
    }

    @PostMapping("/notrash")
    public ResponseResult<Void> notrash(@RequestBody UpdateStatus updateStatus) {
        if (updateStatus == null || updateStatus.getMappingId() == null || updateStatus.getMappingId() <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "invalid mappingId");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return emailService.moveMail(currentUser.getData(), updateStatus.getMappingId(), 2L);
    }

    @PostMapping("/noadvertisement")
    public ResponseResult<Void> noadvertisement(@RequestBody UpdateStatus updateStatus) {
        if (updateStatus == null || updateStatus.getMappingId() == null || updateStatus.getMappingId() <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "invalid mappingId");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return emailService.moveMail(currentUser.getData(), updateStatus.getMappingId(), 2L);
    }

    @GetMapping("/search/suggest")
    public ResponseResult<List<SearchSuggestionVO>> suggest(@RequestParam("keyword") String keyword, @RequestParam(name = "limit", defaultValue = "8") Integer limit) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (keyword == null || keyword.trim().isEmpty()) return ResponseResult.success(Collections.emptyList());
        List<SearchSuggestionVO> result = emailService.getSearchSuggestions(currentUser.getData(), keyword.trim(), limit);
        return ResponseResult.success(result);
    }
    private Long readLongParam(HttpServletRequest request, String name) throws IOException {
        Object value = readParam(request, name);
        if (value == null) return null;
        if (value instanceof Number number) return number.longValue();
        String text = value.toString();
        if (text.isBlank()) return null;
        return Long.parseLong(text);
    }

    private Integer readIntegerParam(HttpServletRequest request, String name) throws IOException {
        Object value = readParam(request, name);
        if (value == null) return null;
        if (value instanceof Number number) return number.intValue();
        String text = value.toString();
        if (text.isBlank()) return null;
        return Integer.parseInt(text);
    }

    private Object readParam(HttpServletRequest request, String name) throws IOException {
        String queryValue = request.getParameter(name);
        if (queryValue != null) return queryValue;

        Object cachedBodyParams = request.getAttribute(DOWNLOAD_BODY_PARAMS_ATTRIBUTE);
        if (cachedBodyParams instanceof Map<?, ?> bodyMap) return bodyMap.get(name);

        String body = request.getReader().lines().reduce("", (left, right) -> left + right);
        if (body.isBlank()) return null;

        Map<String, Object> bodyMap = objectMapper.readValue(body, new TypeReference<>() {});
        request.setAttribute(DOWNLOAD_BODY_PARAMS_ATTRIBUTE, bodyMap);
        return bodyMap.get(name);
    }

    private String buildBaseUrl(HttpServletRequest request) {
        String forwardedProto = firstHeaderValue(request.getHeader("X-Forwarded-Proto"));
        String forwardedHost = firstHeaderValue(request.getHeader("X-Forwarded-Host"));
        String proto = forwardedProto == null || forwardedProto.isBlank() ? request.getScheme() : forwardedProto;
        String host = forwardedHost == null || forwardedHost.isBlank() ? request.getHeader("Host") : forwardedHost;
        if (host == null || host.isBlank()) host = request.getServerName() + ":" + request.getServerPort();
        return proto + "://" + host;
    }

    private String firstHeaderValue(String value) {
        if (value == null) return null;
        int commaIndex = value.indexOf(',');
        return commaIndex >= 0 ? value.substring(0, commaIndex).trim() : value.trim();
    }
}
