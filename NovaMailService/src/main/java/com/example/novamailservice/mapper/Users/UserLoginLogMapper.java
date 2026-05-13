package com.example.novamailservice.mapper.Users;

import com.example.novamailservice.entity.Users.UserLoginLog;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface UserLoginLogMapper {
    @Insert("""
        INSERT INTO UserLoginLog (
            account, loginTime,
            clientIp, ipLocation, networkType, carrier,
            userAgent, deviceType,
            osFamily, osVersion, osMajor, osMinor,
            browser, browserVersion, browserMajor,
            deviceBrand, deviceModel, deviceCode,
            isBot, platform,
            loginResult, failReason, responseTime
        ) VALUES (
            #{account}, #{loginTime},
            #{clientIp}, #{ipLocation}, #{networkType}, #{carrier},
            #{userAgent}, #{deviceType},
            #{osFamily}, #{osVersion}, #{osMajor}, #{osMinor},
            #{browser}, #{browserVersion}, #{browserMajor},
            #{deviceBrand}, #{deviceModel}, #{deviceCode},
            #{isBot}, #{platform},
            #{loginResult}, #{failReason}, #{responseTime}
        )
    """)
    int insertLoginLog(UserLoginLog log);

    @Select("SELECT * FROM UserLoginLog ORDER BY loginTime DESC")
    List<UserLoginLog> selectLoginLog();

    @Select("SELECT * FROM UserLoginLog WHERE account = #{account} ORDER BY loginTime DESC LIMIT #{maxLine}")
    List<UserLoginLog> selectLoginLogByUserId(@Param("account")String account, @Param("maxLine") Long maxLine);

    @Delete("DELETE FROM UserLoginLog WHERE logId = #{logId}")
    int deleteLoginLog(@Param("logId")Long logId);
}
