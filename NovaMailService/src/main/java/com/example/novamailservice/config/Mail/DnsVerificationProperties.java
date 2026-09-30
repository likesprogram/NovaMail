package com.example.novamailservice.config.Mail;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;

import java.util.ArrayList;
import java.util.List;

@Data
@Configuration
@ConfigurationProperties(prefix = "mail.dns-verification")
public class DnsVerificationProperties {
    private List<String> imapA = new ArrayList<>();
    private List<String> smtpA = new ArrayList<>();
    private String mxHost;
    private List<String> spfIp4 = new ArrayList<>();
}
