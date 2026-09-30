package com.example.novamailservice.dto.System;

import lombok.Data;

@Data
public class SystemServiceLoadInfo {
    private Integer cpuPct;
    private Integer memPct;
    private Long stressIndicators;
    private Long delayMs;
    private Double cpu;
    private Double mem;
    private Double stress;
    private Double lat;
    private Double load;
    private Double effectiveLoad;
}
