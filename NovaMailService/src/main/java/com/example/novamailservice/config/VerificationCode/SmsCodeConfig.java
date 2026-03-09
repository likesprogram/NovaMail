package com.example.novamailservice.config.VerificationCode;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;

@Data
@Configuration
@ConfigurationProperties(prefix = "verification-code.sms-code")
public class SmsCodeConfig {
    // 短信验证码配置
    private String accessKeyId;
    // 短信验证码密钥
    private String accessKeySecret;
    // 是否是商业版签名
    private boolean commercial;
    // 短信验证码签名
    private String signName;
    // 短信验证码模板
    private String templateCode;
    // 短信验证码字符个数
    private int codeLength;
    // 短信验证码字符
    private String character;
    // 短信验证码过期时间
    private int expiration;
    // ip发送间隔
    private int ipSendInterval;
    // 手机号发送间隔
    private int phoneSendInterval;
}