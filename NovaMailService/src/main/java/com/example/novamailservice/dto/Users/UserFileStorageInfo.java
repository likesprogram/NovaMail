package com.example.novamailservice.dto.Users;

import lombok.Data;

@Data
public class UserFileStorageInfo {
    private Long fileId;
    private Long userId;
    private String account;
    private String fileName;
    private String fileType;
    private String storageBucket;
    private Long fileSize;
    private String fileStatus;
}
