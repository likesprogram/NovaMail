package com.example.novamailservice.dto.DomainNames;

import lombok.Data;

@Data
public class DomainDkimConfig {
    private Long domainNameId;
    private String domainName;
    private String selector;
    private String recordType;
    private String recordName;
    private String recordValue;
    private Boolean hasDKIM;
}
