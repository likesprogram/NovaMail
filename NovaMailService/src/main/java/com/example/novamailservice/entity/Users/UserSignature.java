package com.example.novamailservice.entity.Users;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class UserSignature {
    Long signatureId;           // 签名ID
    Long userId;                // 所属用户
    String signatureName;       // 签名名称（标题）
    String signatureContent;    // 签名内容
    boolean isDefault;          // 是否是默认选中
    boolean editable = true;    // 可编辑
    LocalDateTime createTime;   // 创建时间
    LocalDateTime updateTime;   // 更新时间
}
