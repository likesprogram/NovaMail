package com.example.novamailservice.service.Mail;

import com.example.novamailservice.dto.MailBoxAddressToMailBoxAndUser;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.DomainNames.DomainNames;
import com.example.novamailservice.entity.Mail.MailBoxChangeLog;
import com.example.novamailservice.entity.Mail.MailBoxes;
import com.example.novamailservice.entity.Users.Users;

import java.util.List;

public interface MailBoxesService {
    // 创建邮箱
    ResponseResult<MailBoxes> createMailBox(Users user, DomainNames domainName, String mailBoxName, int retry);
    // 获取邮箱
    ResponseResult<MailBoxes> getMailBoxByName(Long domainNameId, String mailBoxName);
    // 获取邮箱
    ResponseResult<MailBoxes> getMailBoxById(Long mailBoxId);
    // 获取邮箱列表
    ResponseResult<List<MailBoxes>> getMailBoxListByUserId(Long userId);
    ResponseResult<List<MailBoxes>> getMailBoxListByDomainNameId(Long domainNameId);
    // 查询所有邮箱
    ResponseResult<List<MailBoxes>> getAllMailBoxes();
    // 根据邮箱ID获取完整邮箱地址
    ResponseResult<String> getMailBoxAddress(Long mailBoxId);
    // 根据邮箱对象获取完整邮箱地址
    ResponseResult<String> getMailBoxAddress(MailBoxes mailBox);
    // 根据邮箱地址获取账号
    ResponseResult<MailBoxAddressToMailBoxAndUser> getAccountByMailBoxAddress(String mailBoxAddress);
}
