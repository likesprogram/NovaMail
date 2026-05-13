package com.example.novamailservice.entity.System;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class SystemSettings {
    private Long settingId;

    private String settingName;
    private String settingValue;
    private String settingType;

    private LocalDateTime createTime;
    private LocalDateTime updateTime;
}
