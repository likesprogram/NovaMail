package com.example.novamailservice.util;

import com.example.novamailservice.dto.DeviceInfo;
import org.apache.tika.utils.StringUtils;
import org.springframework.stereotype.Component;
import nl.basjes.parse.useragent.*;

import java.util.Arrays;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

// 设备信息解析工具
@Component
public class DeviceInfoParser {
    private static final UserAgentAnalyzer UA_ANALYZER = UserAgentAnalyzer
            .newBuilder()
            .hideMatcherLoadStats() // 不打印加载日志
            .withCache(10000)       // 缓存10000个UA结果
            .build();
    private static final Set<String> BOT_KEYWORDS = Set.of("bot", "crawl", "spider");

    /**
     * 全功能设备解析入口
     * @param userAgent 浏览器UA字符串
     * @param headers 可选HTTP头（用于网络信息增强）
     */
    public static DeviceInfo parse(String userAgent, Map<String, String> headers) {
        DeviceInfo info = new DeviceInfo();
        info.setRawUserAgent(userAgent);

        if (StringUtils.isBlank(userAgent)) {
            return defaultUnknownInfo(info);
        }

        try {
            UserAgent agent = UA_ANALYZER.parse(userAgent);

            // 设备基础解析
            parseDeviceBaseInfo(info, agent, userAgent);

            // 操作系统解析
            parseOSInfo(info, agent);

            // 浏览器解析
            parseBrowserInfo(info, agent);

            // 增强数据
            enrichEnhancedInfo(info, headers, userAgent);

        } catch (Exception e) {
            handleParseError(info, e);
        }

        return info;
    }

    // 设备基础解析
    private static void parseDeviceBaseInfo(DeviceInfo info, UserAgent agent, String ua) {
        // 获取品牌和型号
        String brand = agent.getValue("DeviceBrand");
        String model = agent.getValue("DeviceName");
        String deviceClass = agent.getValue("DeviceClass");

        info.setDeviceBrand(firstNonEmpty(brand, "Unknown"));
        info.setDeviceModel(firstNonEmpty(model, "Generic"));
        info.setDeviceType(determineDeviceType(deviceClass, ua.toLowerCase()));
        info.setBot(isBotUA(ua));
    }

    // 设备类型判断
    private static DeviceInfo.DeviceType determineDeviceType(String deviceClass, String lowerUA) {
        // 机器人检测
        if (isBotUA(lowerUA)) {
            return DeviceInfo.DeviceType.BOT;
        }

        // 特殊设备检测
        if (lowerUA.contains("xbox") || lowerUA.contains("playstation")) {
            return DeviceInfo.DeviceType.GAME_CONSOLE;
        }
        if (lowerUA.contains("smarttv") || lowerUA.matches(".*tv[^a-z].*")) {
            return DeviceInfo.DeviceType.SMART_TV;
        }

        // 移动设备判断
        if ("MOBILE".equalsIgnoreCase(deviceClass)) {
            return isTablet(lowerUA) ? DeviceInfo.DeviceType.TABLET : DeviceInfo.DeviceType.MOBILE;
        }
        if ("TABLET".equalsIgnoreCase(deviceClass)) {
            return DeviceInfo.DeviceType.TABLET;
        }
        if ("DESKTOP".equalsIgnoreCase(deviceClass)) {
            return DeviceInfo.DeviceType.DESKTOP;
        }

        // 其他设备类型
        if (lowerUA.contains("watch") || lowerUA.contains("glass")) {
            return DeviceInfo.DeviceType.WEARABLE;
        }
        if (lowerUA.contains("kindle") || lowerUA.contains("nook")) {
            return DeviceInfo.DeviceType.E_READER;
        }

        // 默认返回未知类型
        return DeviceInfo.DeviceType.UNKNOWN;
    }

    // 操作系统解析
    private static void parseOSInfo(DeviceInfo info, UserAgent agent) {
        String osFamily = agent.getValue("OperatingSystemName");
        String osMajor = agent.getValue("OperatingSystemVersionMajor");
        String osMinor = agent.getValue("OperatingSystemVersionMinor");
        String osPatch = agent.getValue("OperatingSystemVersionMicro");

        // 操作系统信息
        info.setOsFamily(firstNonEmpty(osFamily, "Unknown"));
        info.setOsMajor(firstNonEmpty(osMajor, "0"));
        info.setOsMinor(firstNonEmpty(osMinor, "0"));
        info.setOsVersion(formatVersion(osMajor, osMinor, osPatch));

        // 平台推断
        info.setPlatform(mapPlatform(info.getOsFamily()));
    }

    // 浏览器解析
    private static void parseBrowserInfo(DeviceInfo info, UserAgent agent) {
        String browserFamily = agent.getValue("AgentName");
        String browserMajor = agent.getValue("AgentVersionMajor");
        String browserMinor = agent.getValue("AgentVersionMinor");
        String browserPatch = agent.getValue("AgentVersionMicro");

        info.setBrowser(firstNonEmpty(browserFamily, "Unknown"));
        info.setBrowserMajor(firstNonEmpty(browserMajor, "0"));
        info.setBrowserVersion(formatVersion(browserMajor, browserMinor, browserPatch));
    }

    // 数据增强方法
    private static void enrichEnhancedInfo(DeviceInfo info, Map<String, String> headers, String ua) {
        // 苹果设备代码映射
        if ("Apple".equalsIgnoreCase(info.getDeviceBrand())) {
            info.setDeviceCode(AppleDeviceMapper.lookup(info.getDeviceModel()));
        }

        // 网络信息（需前端配合）
        if (headers != null) {
            info.setNetworkType(headers.getOrDefault("X-Network-Type",
                    detectNetworkTypeFromUA(ua)));
            info.setCarrier(headers.getOrDefault("X-Carrier", "Unknown"));
        }

        // 机器人二次验证
        info.setBot(info.getDeviceType() == DeviceInfo.DeviceType.BOT ||
                isBotUA(info.getRawUserAgent()));
    }

    // 工具方法
    private static String formatVersion(String major, String minor, String patch) {
        return String.join(".",
                Objects.requireNonNullElse(major, "0"),
                Objects.requireNonNullElse(minor, "0"),
                Objects.requireNonNullElse(patch, "0")
        ).replaceAll("(\\.0)+$", "");
    }

    private static String mapPlatform(String osFamily) {
        if (osFamily == null) return "Unknown";
        return switch (osFamily.split(" ")[0]) {
            case "iOS" -> "iOS";
            case "Android" -> "Android";
            case "Windows" -> "Windows";
            case "Mac" -> "macOS";
            case "Linux" -> "Linux";
            default -> "Other";
        };
    }

    private static boolean isTablet(String lowerUA) {
        return lowerUA.contains("ipad") ||
                (lowerUA.contains("android") && !lowerUA.contains("mobile"));
    }

    private static boolean isBotUA(String ua) {
        return ua != null && BOT_KEYWORDS.stream().anyMatch(ua.toLowerCase()::contains);
    }

    private static String detectNetworkTypeFromUA(String ua) {
        if (ua == null) return "Unknown";
        if (ua.contains("Mobile")) return "4G/5G";
        if (ua.contains("WIFI")) return "WiFi";
        return "Broadband";
    }

    private static String firstNonEmpty(String... values) {
        return Arrays.stream(values)
                .filter(Objects::nonNull)
                .filter(s -> !s.isEmpty())
                .findFirst()
                .orElse("");
    }

    // 错误处理
    private static DeviceInfo defaultUnknownInfo(DeviceInfo info) {
        info.setDeviceType(DeviceInfo.DeviceType.UNKNOWN);
        info.setOsFamily("Unknown");
        info.setBrowser("Unknown");
        return info;
    }

    private static void handleParseError(DeviceInfo info, Exception e) {
        info.setDeviceType(DeviceInfo.DeviceType.UNKNOWN);
        info.setOsFamily("Parse Error");
        info.setDeviceBrand("Error: " + e.getClass().getSimpleName());
    }

    // 内部映射类
    private static class AppleDeviceMapper {
        private static final Map<String, String> MODEL_MAP = Map.ofEntries(
                Map.entry("iPhone 15 Pro", "iPhone16,2"),
                Map.entry("iPad Pro 11\"", "iPad14,3"),
                Map.entry("MacBook Pro", "Mac14,10")
        );

        static String lookup(String model) {
            return MODEL_MAP.entrySet().stream()
                    .filter(e -> model.contains(e.getKey()))
                    .findFirst()
                    .map(Map.Entry::getValue)
                    .orElse("unknown");
        }
    }
}
