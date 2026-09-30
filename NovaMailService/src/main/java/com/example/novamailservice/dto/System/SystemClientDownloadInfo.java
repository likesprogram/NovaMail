package com.example.novamailservice.dto.System;

import lombok.Data;

@Data
public class SystemClientDownloadInfo {
    private String fileName;
    private String fileType;
    private Long fileSize;
    private String version;
    private String description;
    private String storageBucket;
    private String objectName;
    private String uploadTime;
}
