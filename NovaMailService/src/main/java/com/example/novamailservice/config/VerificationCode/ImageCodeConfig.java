package com.example.novamailservice.config.VerificationCode;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;

@Data
@Configuration
@ConfigurationProperties(prefix = "verification-code.image-code")
public class ImageCodeConfig {
    // 图片验证码配置
    private int width;
    // 图片高度
    private int height;
    // 验证码字符个数
    private int codeLength;
    // 字体名称
    private String fontName;
    // 字体大小
    private int fontSize;
    // 验证码字符
    private String character;
    // 验证码过期时间 单位秒
    private int expiration;
}
