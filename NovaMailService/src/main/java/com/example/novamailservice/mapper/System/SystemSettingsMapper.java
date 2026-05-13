package com.example.novamailservice.mapper.System;

import com.example.novamailservice.entity.System.SystemSettings;
import org.apache.ibatis.annotations.*;

@Mapper
public interface SystemSettingsMapper {
    @Insert("INSERT INTO SystemSettings(settingName, settingValue, settingType) " +
            "VALUES (#{settingName}, #{settingValue}, #{settingType}) AS new " +
            "ON DUPLICATE KEY UPDATE settingValue = new.settingValue, settingType = new.settingType")
    int setSystemSettings(@Param("settingName")String settingName, @Param("settingValue")String settingValue, @Param("settingType")String settingType);

    @Select("SELECT * FROM SystemSettings WHERE settingName = #{settingName}")
    SystemSettings getSystemSettings(@Param("settingName")String settingName);
}
