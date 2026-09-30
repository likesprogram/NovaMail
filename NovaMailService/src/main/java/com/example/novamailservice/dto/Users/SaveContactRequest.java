package com.example.novamailservice.dto.Users;

import lombok.Data;

@Data
public class SaveContactRequest {
    private Long contactId;
    private Long groupId;
    private String contactName;
    private String email;
    private String phone;
    private String remark;
}
