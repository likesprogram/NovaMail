package com.example.novamailservice.dto.System;

import lombok.Data;

@Data
public class SystemServiceMonitorInfo {
    private String serviceId;
    private String serviceName;
    private String serviceAddress;
    private Integer lifecycle;
    private String lifecycleText;
    private Integer health;
    private String healthText;
    private SystemServiceLoadInfo load;
    private String lastUpdateTime;
    private Boolean currentNode;
    private String source;
}
