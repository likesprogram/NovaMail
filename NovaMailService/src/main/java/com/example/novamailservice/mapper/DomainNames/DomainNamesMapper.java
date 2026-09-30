package com.example.novamailservice.mapper.DomainNames;

import com.example.novamailservice.entity.DomainNames.DomainNames;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface DomainNamesMapper {
    // 添加域名
    @Insert("INSERT INTO DomainNames(domainName, userId, hasCertificate, hasDKIM) " +
            "VALUES(#{domainName}, #{userId}, #{hasCertificate}, #{hasDKIM})")
    @Options(useGeneratedKeys = true, keyProperty = "domainNameId")
    int createDomainName(DomainNames domainNames);

    // 获取所有域名（管理员）
    @Select("SELECT * FROM DomainNames ORDER BY isDefault DESC, userId IS NULL DESC, updateTime DESC")
    List<DomainNames> selectAll();

    // 获取系统默认域名
    @Select("SELECT * FROM DomainNames WHERE isDefault = 1 LIMIT 1")
    DomainNames getSystemDefaultDomainName();

    // 清除默认域名
    @Update("UPDATE DomainNames SET isDefault = 0 WHERE isDefault = 1")
    int clearDefault();

    // 设置默认域名
    @Update("UPDATE DomainNames SET isDefault = 1 WHERE domainNameId = #{domainNameId}")
    int setDefault(Long domainNameId);

    // 通过域名ID获取域名
    @Select("SELECT * FROM DomainNames WHERE domainNameId = #{domainNameId}")
    DomainNames selectById(@Param("domainNameId")Long domainNameId);

    // 通过域名名称获取域名
    @Select("SELECT * FROM DomainNames WHERE domainName = #{domainName}")
    DomainNames selectByName(@Param("domainName")String domainName);

    // 通过用户ID获取域名
    @Select("SELECT * FROM DomainNames WHERE userId = #{userId}")
    List<DomainNames> selectByUserId(@Param("userId")Long userId);

    // 获取当前用户可使用的域名：公共域名、自己的域名、授权给自己的域名
    @Select("""
            SELECT DISTINCT dn.*
            FROM DomainNames dn
            LEFT JOIN DomainNameAuthorization dna
              ON dna.domainNameId = dn.domainNameId
             AND dna.userId = #{userId}
             AND dna.`permission` IN ('USE', 'ADMIN')
             AND (dna.expireTime IS NULL OR dna.expireTime > CURRENT_TIMESTAMP)
            WHERE dn.userId IS NULL
               OR dn.userId = #{userId}
               OR dna.authorizationId IS NOT NULL
            ORDER BY dn.isDefault DESC, dn.userId IS NULL DESC, dn.updateTime DESC
            """)
    List<DomainNames> selectAvailableByUserId(@Param("userId")Long userId);

    // 查询有效授权权限
    @Select("""
            SELECT `permission`
            FROM DomainNameAuthorization
            WHERE domainNameId = #{domainNameId}
              AND userId = #{userId}
              AND `permission` IN ('USE', 'ADMIN')
              AND (expireTime IS NULL OR expireTime > CURRENT_TIMESTAMP)
            ORDER BY CASE `permission` WHEN 'ADMIN' THEN 0 ELSE 1 END
            LIMIT 1
            """)
    String selectActivePermission(@Param("domainNameId")Long domainNameId, @Param("userId")Long userId);

    // 更新当前用户自己的域名
    @Update("UPDATE DomainNames SET domainName = #{domainName}, hasCertificate = #{hasCertificate}, hasDKIM = #{hasDKIM} " +
            "WHERE domainNameId = #{domainNameId} AND userId = #{userId}")
    int updateOwnedDomainName(DomainNames domainNames);

    // 管理员更新域名基础信息
    @Update("UPDATE DomainNames SET domainName = #{domainName}, userId = #{userId}, hasCertificate = #{hasCertificate}, hasDKIM = #{hasDKIM} " +
            "WHERE domainNameId = #{domainNameId}")
    int updateDomainName(DomainNames domainNames);

    // 更新任意域名 DKIM 状态
    @Update("UPDATE DomainNames SET hasDKIM = #{hasDKIM} WHERE domainNameId = #{domainNameId}")
    int updateDkimStatus(@Param("domainNameId")Long domainNameId, @Param("hasDKIM")Boolean hasDKIM);

    // 更新任意域名 TLS 证书状态
    @Update("UPDATE DomainNames SET hasCertificate = #{hasCertificate} WHERE domainNameId = #{domainNameId}")
    int updateCertificateStatus(@Param("domainNameId")Long domainNameId, @Param("hasCertificate")Boolean hasCertificate);

    // 更新当前用户自己的 DKIM 状态
    @Update("UPDATE DomainNames SET hasDKIM = #{hasDKIM} WHERE domainNameId = #{domainNameId} AND userId = #{userId}")
    int updateOwnedDkimStatus(@Param("domainNameId")Long domainNameId, @Param("userId")Long userId, @Param("hasDKIM")Boolean hasDKIM);

    // 更新当前用户自己的 TLS 证书状态
    @Update("UPDATE DomainNames SET hasCertificate = #{hasCertificate} WHERE domainNameId = #{domainNameId} AND userId = #{userId}")
    int updateOwnedCertificateStatus(@Param("domainNameId")Long domainNameId, @Param("userId")Long userId, @Param("hasCertificate")Boolean hasCertificate);

    // 删除当前用户自己的域名
    @Delete("DELETE FROM DomainNames WHERE domainNameId = #{domainNameId} AND userId = #{userId}")
    int deleteOwnedDomainName(@Param("domainNameId")Long domainNameId, @Param("userId")Long userId);

    // 管理员删除域名
    @Delete("DELETE FROM DomainNames WHERE domainNameId = #{domainNameId}")
    int deleteDomainName(@Param("domainNameId")Long domainNameId);
}
