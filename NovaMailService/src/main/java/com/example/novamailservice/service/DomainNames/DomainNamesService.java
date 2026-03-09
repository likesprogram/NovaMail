package com.example.novamailservice.service.DomainNames;

import com.example.novamailservice.dto.Result.ResponseResult;
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
}
