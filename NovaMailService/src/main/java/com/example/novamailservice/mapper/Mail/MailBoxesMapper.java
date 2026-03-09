package com.example.novamailservice.mapper.Mail;

import com.example.novamailservice.entity.Mail.MailBoxes;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface MailBoxesMapper {
    // 创建邮箱
    @Insert("INSERT INTO MailBoxes(userId, domainNameId, localPart, isOpen) " +
            "VALUES(#{userId}, #{domainNameId}, #{localPart}, #{isOpen})")
    @Options(useGeneratedKeys = true, keyProperty = "mailBoxId")
    int createMailBox(MailBoxes mailBoxes);

    // 通过域名和邮箱名称获取邮箱
    @Select("SELECT * FROM MailBoxes WHERE domainNameId = #{domainNameId} AND localPart = #{mailBoxName}")
    MailBoxes selectByDomainAndAddress(@Param("domainNameId")Long domainNameId, @Param("mailBoxName")String mailBoxName);

    // 通过邮箱ID获取邮箱
    @Select("SELECT * FROM MailBoxes WHERE mailBoxId = #{mailBoxId}")
    MailBoxes selectById(@Param("mailBoxId")Long mailBoxId);

    // 通过用户ID获取邮箱
    @Select("SELECT * FROM MailBoxes WHERE userId = #{userId}")
    List<MailBoxes> selectByUserId(@Param("userId")Long userId);

    // 通过域名ID获取邮箱
    @Select("SELECT * FROM MailBoxes WHERE domainNameId = #{domainNameId}")
    List<MailBoxes> selectByDomainId(@Param("domainNameId")Long domainNameId);
}
