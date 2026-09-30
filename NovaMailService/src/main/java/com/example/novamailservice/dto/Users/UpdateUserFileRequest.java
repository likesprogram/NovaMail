package com.example.novamailservice.dto.Users;

import lombok.Data;

@Data
public class UpdateUserFileRequest {
    private Long fileId;
    private String fileName;
    private String description;
}
