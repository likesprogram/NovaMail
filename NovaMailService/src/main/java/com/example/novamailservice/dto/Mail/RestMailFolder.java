package com.example.novamailservice.dto.Mail;

import lombok.Data;

@Data
public class RestMailFolder {
    private Long folderId;
    private String folderName;
    private Long noReadCont;
}
