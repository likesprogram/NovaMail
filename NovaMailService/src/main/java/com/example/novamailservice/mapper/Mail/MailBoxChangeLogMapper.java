package com.example.novamailservice.mapper.Mail;

import com.example.novamailservice.entity.Mail.MailBoxChangeLog;
import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Options;

@Mapper
public interface MailBoxChangeLogMapper {
    @Insert("INSERT INTO MailBoxChangeLog (mailBoxId, userId, action, oldValue, newValue, remark) " +
            "VALUES (#{mailBoxId}, #{userId}, #{action}, #{oldValue}, #{newValue}, #{remark})")
    @Options(useGeneratedKeys = true, keyProperty = "logId")
    int insert(MailBoxChangeLog record);
}
