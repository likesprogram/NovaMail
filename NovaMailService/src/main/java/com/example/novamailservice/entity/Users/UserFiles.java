package com.example.novamailservice.entity.Users;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class UserFiles {
    private Long fileId;
    private Long userId;
    private String fileName;
    private String fileType;
    private String filePath;
    private String parentPath;
    private String storageBucket;
    private String fileHash;
    private Long fileSize;
    private Integer version;
    private String fileStatus;
    private String description;
    private LocalDateTime uploadTime;
    private LocalDateTime updateTime;
}
