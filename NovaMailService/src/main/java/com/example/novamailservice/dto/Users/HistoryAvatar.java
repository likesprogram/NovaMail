package com.example.novamailservice.dto.Users;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.time.LocalDateTime;

@Data
@AllArgsConstructor
public class HistoryAvatar {
    private String avatarName;          // 头像文件名
    private String avatarBase64;        // 头像 Base64 数据
    private Boolean isCurrent;          // 是否是当前使用
    private LocalDateTime uploadTime;   // 上传时间
}
