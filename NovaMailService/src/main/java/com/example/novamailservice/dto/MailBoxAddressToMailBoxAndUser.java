package com.example.novamailservice.dto;

import com.example.novamailservice.entity.DomainNames.DomainNames;
import com.example.novamailservice.entity.Mail.MailBoxes;
import com.example.novamailservice.entity.Users.Users;
import lombok.Data;

@Data
public class MailBoxAddressToMailBoxAndUser {
    private DomainNames domainName;
    private Users user;
    private MailBoxes mailBox;
}
