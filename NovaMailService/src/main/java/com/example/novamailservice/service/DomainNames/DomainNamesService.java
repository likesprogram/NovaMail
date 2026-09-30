package com.example.novamailservice.service.DomainNames;

import com.example.novamailservice.dto.DomainNames.DomainDkimConfig;
import com.example.novamailservice.dto.DomainNames.DomainDnsVerification;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.DomainNames.DomainCredentials;
import com.example.novamailservice.entity.DomainNames.DomainNames;

import java.util.List;

public interface DomainNamesService {
    // 获取系统默认域名
    ResponseResult<DomainNames> getSystemDefaultDomainName();
    // 通过域名ID获取域名
    ResponseResult<DomainNames> getDomainNameById(Long domainNameId);
    // 通过域名名称获取域名
    ResponseResult<DomainNames> getDomainNameByName(String domainName);
    // 通过用户ID获取域名
    ResponseResult<List<DomainNames>> getDomainNameByUserId(Long userId);
    // 获取用户可使用的域名
    ResponseResult<List<DomainNames>> getAvailableDomainNameByUserId(Long userId);
    // 管理员获取全部域名
    ResponseResult<List<DomainNames>> getAllDomainNames();
    // 管理员新增域名
    ResponseResult<DomainNames> createSystemDomainName(DomainNames domainName);
    // 管理员修改域名
    ResponseResult<DomainNames> updateSystemDomainName(DomainNames domainName);
    // 管理员删除域名
    ResponseResult<String> deleteSystemDomainName(Long domainNameId);
    // 管理员设置默认域名
    ResponseResult<String> setSystemDefaultDomainName(Long domainNameId);
    // 新增用户自己的域名
    ResponseResult<DomainNames> createUserDomainName(Long userId, DomainNames domainName);
    // 修改用户自己的域名
    ResponseResult<DomainNames> updateUserDomainName(Long userId, DomainNames domainName);
    // 删除用户自己的域名
    ResponseResult<String> deleteUserDomainName(Long userId, Long domainNameId);
    // 当前用户是否可使用该域名
    boolean canUseDomain(DomainNames domainName, Long userId);
    // 当前用户对域名的权限标签
    String getUserDomainPermission(DomainNames domainName, Long userId);
    // 获取用户自己域名的 DKIM DNS 配置
    ResponseResult<DomainDkimConfig> getUserDomainDkimConfig(Long userId, Long domainNameId);
    // 配置用户自己域名的 TLS 证书
    ResponseResult<String> configureUserDomainTls(Long userId, DomainCredentials domainCredentials);
    // 验证用户自己域名的 DNS 解析
    ResponseResult<DomainDnsVerification> verifyUserDomainDns(Long userId, Long domainNameId);
    // 管理员获取域名 DKIM DNS 配置
    ResponseResult<DomainDkimConfig> getSystemDomainDkimConfig(Long domainNameId);
    // 管理员配置域名 TLS 证书
    ResponseResult<String> configureSystemDomainTls(DomainCredentials domainCredentials);
    // 管理员验证域名 DNS 解析
    ResponseResult<DomainDnsVerification> verifySystemDomainDns(Long domainNameId);
}
