package com.example.novamailservice.dto.DomainNames;

import com.example.novamailservice.entity.DomainNames.DomainNames;
import lombok.Data;

import java.time.LocalDateTime;

@Data
public class ResultDomainName {
    private Long domainNameId;
    private String domainName;
    private Long userId;
    private Boolean hasCertificate;
    private Boolean hasDKIM;
    private Boolean isDefault;
    private LocalDateTime updateTime;
    private String permission;
    private Boolean editable;

    public static ResultDomainName fromDomainName(DomainNames domainName, Long currentUserId, String permission) {
        ResultDomainName result = new ResultDomainName();
        result.setDomainNameId(domainName.getDomainNameId());
        result.setDomainName(domainName.getDomainName());
        result.setUserId(domainName.getUserId());
        result.setHasCertificate(domainName.getHasCertificate());
        result.setHasDKIM(domainName.getHasDKIM());
        result.setIsDefault(domainName.getIsDefault());
        result.setUpdateTime(domainName.getUpdateTime());
        result.setPermission(permission);
        result.setEditable(domainName.getUserId() != null && domainName.getUserId().equals(currentUserId));
        return result;
    }
}
