package com.example.novamailservice.dto;

import lombok.Data;

@Data
public class IpInfo {
    private String ipAddress; // IP地址 192.168.0.1
    private String ipRegion;  // IP归属地 国家|区域|省份|城市|ISP
}
