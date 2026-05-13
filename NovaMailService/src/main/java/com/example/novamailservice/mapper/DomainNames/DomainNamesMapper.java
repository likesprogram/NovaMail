package com.example.novamailservice.mapper.DomainNames;

import com.example.novamailservice.entity.DomainNames.DomainNames;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface DomainNamesMapper {
    // 添加域名
    @Insert("INSERT INTO DomainNames(domainName, userId, hasCertificate, hasDKIM) " +
            "VALUES(#{domainName}, #{userId}, #{hasCertificate}, #{hasDKIM})")
    int createDomainName(DomainNames domainNames);

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
}
