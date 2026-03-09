package com.example.novamailservice.dto;

import lombok.Data;

@Data
public class DeviceInfo {
    public enum DeviceType {
        MOBILE("手机", true),          // 智能手机
        TABLET("平板", true),          // iPad等平板设备
        DESKTOP("桌面电脑", false),    // PC/Mac
        SMART_TV("智能电视", false),   // 电视设备
        WEARABLE("穿戴设备", true),    // 智能手表/眼镜
        GAME_CONSOLE("游戏主机", false), // PS5/Xbox
        E_READER("电子书", false),     // Kindle
        BOT("爬虫程序", false),        // 搜索引擎爬虫
        UNKNOWN("未知设备", false);    // 兜底类型

        private final String displayName;
        private final boolean isPortable; // 是否便携设备

        DeviceType(String displayName, boolean isPortable) {
            this.displayName = displayName;
            this.isPortable = isPortable;
        }

        // 判断是否为移动设备（手机+平板）
        public boolean isMobileDevice() {
            return this == MOBILE || this == TABLET;
        }

        public String getDisplayName() {
            return displayName;
        }

        public boolean isPortable() {
            return isPortable;
        }
    }

    // 基础信息
    private String rawUserAgent;
    private DeviceType deviceType; // MOBILE, DESKTOP, etc.

    // 操作系统
    private String osFamily;       // e.g. "iOS"
    private String osVersion;      // e.g. "15.5"
    private String osMajor;        // e.g. "15"
    private String osMinor;        // e.g. "5"

    // 浏览器
    private String browser;        // e.g. "Chrome Mobile"
    private String browserVersion; // e.g. "103.0.5060"
    private String browserMajor;   // e.g. "103"

    // 设备详情
    private String deviceBrand;    // e.g. "Apple"
    private String deviceModel;    // e.g. "iPhone 13 Pro"
    private String deviceCode;     // e.g. "iPhone14,2"

    // 网络信息
    private String networkType;    // 需额外解析 (e.g. "4G")
    private String carrier;        // 需额外解析 (e.g. "China Mobile")

    // 扩展元数据
    private boolean isBot;         // 是否爬虫
    private String platform;       // e.g. "Web/iOS/Android"
}
