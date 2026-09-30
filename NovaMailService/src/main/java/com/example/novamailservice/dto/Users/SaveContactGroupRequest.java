package com.example.novamailservice.dto.Users;

import lombok.Data;

@Data
public class SaveContactGroupRequest {
    private Long groupId;
    private String groupName;
}
