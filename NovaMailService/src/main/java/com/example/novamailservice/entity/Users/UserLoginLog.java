package com.example.novamailservice.entity.Users;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class UserLoginLog {
    private Long logId;
    private String account; // null 表示用户不存在（撞库/爆破）
    private LocalDateTime loginTime;

    // 网络信息
    private String clientIp;
    private String ipLocation;
    private String networkType;
    private String carrier;

    // 设备信息
    private String userAgent;

    private String deviceType;   // PC / Mobile / Tablet

    // 系统信息
    private String osFamily;
    private String osVersion;
    private String osMajor;
    private String osMinor;

    // 浏览器信息
    private String browser;
    private String browserVersion;
    private String browserMajor;

    // 设备详细
    private String deviceBrand;
    private String deviceModel;
    private String deviceCode;   // 设备唯一ID（可为空）

    // 安全与风控
    private Boolean isBot;
    private String platform;     // Web / Android / iOS / Windows / macOS
    private String loginResult;   // SUCCESS / FAIL / LOCKED / EXPIRED
    private String failReason;
    private Integer responseTime; // ms
}
