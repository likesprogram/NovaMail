package com.example.novamailservice.mapper.DomainNames;

import com.example.novamailservice.entity.DomainNames.DomainCredentials;
import org.apache.ibatis.annotations.*;

@Mapper
public interface DomainCredentialsMapper {
    @Select("SELECT * FROM DomainCredentials WHERE domainNameId = #{domainNameId} LIMIT 1")
    DomainCredentials selectByDomainNameId(@Param("domainNameId") Long domainNameId);

    @Insert("""
            INSERT INTO DomainCredentials(domainNameId, privateKey, publicKey, `certificate`, caChain, sigPrivateKey, sigPublicKey)
            VALUES(#{domainNameId}, '', '', '', NULL, #{sigPrivateKey}, #{sigPublicKey})
            ON DUPLICATE KEY UPDATE
                sigPrivateKey = VALUES(sigPrivateKey),
                sigPublicKey = VALUES(sigPublicKey),
                lastRotated = CURRENT_TIMESTAMP
            """)
    int upsertDkimCredentials(DomainCredentials domainCredentials);

    @Insert("""
            INSERT INTO DomainCredentials(domainNameId, privateKey, publicKey, `certificate`, caChain, sigPrivateKey, sigPublicKey)
            VALUES(#{domainNameId}, #{privateKey}, #{publicKey}, #{certificate}, #{caChain}, '', '')
            ON DUPLICATE KEY UPDATE
                privateKey = VALUES(privateKey),
                publicKey = VALUES(publicKey),
                `certificate` = VALUES(`certificate`),
                caChain = VALUES(caChain),
                lastRotated = CURRENT_TIMESTAMP
            """)
    int upsertTlsCredentials(DomainCredentials domainCredentials);
}
