package com.example.novamailservice.entity.DomainNames;

import lombok.Data;

import java.sql.Timestamp;

@Data
public class DomainCredentials {
    // 域名ID
    private Long domainNameId;
    // TLS 私钥 (PEM)
    private String privateKey;
    // TLS 公钥 (PEM)
    private String publicKey;
    // TLS 证书 (PEM)
    private String certificate;
    // 中间证书链 (PEM)
    private String caChain;
    // DKIM 签名私钥
    private String sigPrivateKey;
    // DKIM 签名公钥
    private String sigPublicKey;
    // 最后密钥轮换时间
    private Timestamp lastRotated;
}
