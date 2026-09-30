package com.example.novamailservice.service.Mail;

import com.example.novamailservice.dto.Mail.MailBoxMailMapping;
import com.example.novamailservice.dto.Mail.DownloadAttachmentRequest;
import com.example.novamailservice.dto.Mail.DownloadParsedAttachmentRequest;
import com.example.novamailservice.dto.Mail.MailDetailResponse;
import com.example.novamailservice.dto.Mail.DraftDetailResponse;
import com.example.novamailservice.dto.Mail.RestMailFolder;
import com.example.novamailservice.dto.Mail.SaveDraftResponse;
import com.example.novamailservice.dto.Mail.SearchSuggestionVO;
import com.example.novamailservice.dto.Mail.SendMailAttachmentRequest;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Users.Users;
import jakarta.servlet.http.HttpServletResponse;

import java.util.List;

public interface EmailService {
    // 发送邮件
    ResponseResult<String> sendMail(Long mailBoxId, String sendName, List<String> toAddressList, List<String> ccAddressList, List<String> bccAddressList, String subject, String bodyData, List<SendMailAttachmentRequest> attachments, Long replySourceMappingId, String baseUrl);
    // 保存草稿
    ResponseResult<SaveDraftResponse> saveDraft(Long draftMappingId, Long mailBoxId, String sendName, List<String> toAddressList, List<String> ccAddressList, List<String> bccAddressList, String subject, String bodyData, List<SendMailAttachmentRequest> attachments);
    // 获取草稿
    ResponseResult<DraftDetailResponse> getDraft(Users user, Long mappingId);
    // 获取邮件文件夹列表
    List<RestMailFolder> getMailFolder(Users user);
    // 获取邮件列表
    List<MailBoxMailMapping> getEmailList(Users user, RestMailFolder mailFolder);
    // 获取邮件内容
    MailDetailResponse getMailDetail(Users user, Long mappingId);
    // 设置阅读状态
    boolean setMailReader(Users user, Long mappingId, boolean reader);
    // 设置星标状态
    boolean setStarred(Users user, Long mappingId, boolean starred);
    // 设置回复状态
    boolean setAnswered(Users user, Long mappingId, boolean answered);
    ResponseResult<Void> moveMail(Users user, Long mappingId, Long targetFolderId);
    ResponseResult<Void> deleteMail(Users user, Long mappingId);
    // 搜索
    List<SearchSuggestionVO> getSearchSuggestions(Users user, String keyword, Integer limit);
    void downloadAttachment(Users user, DownloadAttachmentRequest request, HttpServletResponse response) throws Exception;
    void previewAttachment(Users user, DownloadAttachmentRequest request, HttpServletResponse response) throws Exception;
    void downloadParsedAttachment(Users user, DownloadParsedAttachmentRequest request, HttpServletResponse response) throws Exception;
    void previewParsedAttachment(Users user, DownloadParsedAttachmentRequest request, HttpServletResponse response) throws Exception;
}
