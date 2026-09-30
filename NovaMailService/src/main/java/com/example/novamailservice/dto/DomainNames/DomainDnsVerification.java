package com.example.novamailservice.dto.DomainNames;

import lombok.Data;

import java.util.List;

@Data
public class DomainDnsVerification {
    private Long domainNameId;
    private String domainName;
    private Boolean passed;
    private List<RecordCheck> records;

    @Data
    public static class RecordCheck {
        private String key;
        private String label;
        private String host;
        private String recordType;
        private String expectedValue;
        private String actualValue;
        private Boolean passed;
        private String message;
    }
}
