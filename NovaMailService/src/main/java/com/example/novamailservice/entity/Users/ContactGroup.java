package com.example.novamailservice.entity.Users;

import lombok.Data;

import java.time.LocalDateTime;

@Data
public class ContactGroup {
    private Long groupId;
    private Long userId;
    private String groupName;
    private String groupType;
    private Integer sortOrder;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
}
