package com.example.novamailservice.entity.Users;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class Contact {
    private Long contactId;
    private Long userId;
    private Long groupId;
    private String contactName;
    private String email;
    private String phone;
    private String remark;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
}
