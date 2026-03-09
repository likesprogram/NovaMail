package com.example.novamailservice.mapper.DomainNames;

import com.example.novamailservice.entity.DomainNames.DomainNames;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;

import java.util.List;

@Mapper
public interface DomainNamesMapper {
    // 获取系统默认域名
    @Select("SELECT * FROM DomainNames WHERE isDefault = 1")
    DomainNames getSystemDefaultDomainName();

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
