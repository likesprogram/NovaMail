package com.example.novamailservice.service.Mail;

import com.example.novamailservice.dto.Mail.MailBoxMailMapping;
import com.example.novamailservice.dto.Mail.MailDetailResponse;
import com.example.novamailservice.dto.Mail.RestMailFolder;
import com.example.novamailservice.dto.Mail.SearchSuggestionVO;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Users.Users;

import java.util.List;

public interface EmailService {
    // 发送邮件
    ResponseResult<String> sendMail(Long mailBoxId, String sendName, List<String> toAddressList, String subject, String bodyData);
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
    // 搜索
    List<SearchSuggestionVO> getSearchSuggestions(Users user, String keyword, Integer limit);
}
