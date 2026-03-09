package com.example.novamailservice.entity.Mail;

import com.example.novamailservice.dto.Result.ResponseResult;
import lombok.Data;

import java.time.LocalDateTime;

@Data
public class MailBoxes {
    // 邮箱ID
    private Long mailBoxId;
    // 所属用户ID
    private Long userId;
    // 域名ID
    private Long domainNameId;
    // 邮箱地址
    private String localPart;
    // 是否启用(0: 禁用, 1: 启用, 2: 删除)
    private Integer isOpen;
    // 创建时间
    private LocalDateTime createTime;
    // 更新时间
    private LocalDateTime updateTime;

    // 邮箱地址校验
    public static ResponseResult<String> validateMailBoxAddress(String mailBoxAddress) {
        if (mailBoxAddress == null || mailBoxAddress.isEmpty()) return ResponseResult.error("邮箱名称不能为空");
        int nameLength = mailBoxAddress.codePointCount(0, mailBoxAddress.length());
        if (nameLength > 64) return ResponseResult.error("邮箱名称长度不能超过64个字符");
        if (nameLength < 2) return ResponseResult.error("邮箱名称长度不能少于2个字符");
        if (!mailBoxAddress.matches("^[a-zA-Z0-9]+$")) return ResponseResult.error("邮箱名称只能包含字母和数字");
        return ResponseResult.success();
    }
}
