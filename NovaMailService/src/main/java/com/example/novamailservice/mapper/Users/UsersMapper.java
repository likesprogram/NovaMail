package com.example.novamailservice.mapper.Users;

import com.example.novamailservice.entity.Users.Users;
import org.apache.ibatis.annotations.*;

@Mapper
public interface UsersMapper {
    // 注册
    @Insert("INSERT INTO Users(account, password, isAdmin) " +
            "VALUES(#{account}, #{password}, #{isAdmin})")
    @Options(useGeneratedKeys = true, keyProperty = "userId")
    int register(Users user);

    // 通过账号查询
    @Select("SELECT * FROM Users WHERE account = #{account}")
    Users selectByAccount(@Param("account")String account);

    // 通过ID查询
    @Select("SELECT * FROM Users WHERE userId = #{userId}")
    Users selectById(@Param("userId")Integer userId);

    // 查询账号数量
    @Select("SELECT COUNT(account) FROM users")
    int getUserCount();

    // 修改用户信息
    @Update("UPDATE Users SET account = #{account} userName = #{userName}, password = #{password}, isAdmin = #{isAdmin} " +
        "WHERE userId = #{userId}")
    int updateUser(Users user);

    // 写入登录日志
//    @Insert("INSERT INTO user_login_log " +
//            "(user_id, ip_address, ip_region, network_type, carrier, " +
//            "   user_agent, device_type, os_family, os_version, os_major, os_minor, " +
//            "   browser, browser_version, browser_major, device_brand, device_model, device_code, " +
//            "   is_bot, platform, login_result, fail_reason, response_time) " +
//            "VALUES (#{userId}, #{ipAddress}, #{ipRegion}, #{networkType}, #{carrier}, " +
//            "   #{userAgent}, #{deviceType.displayName}, #{osFamily}, #{osVersion}, #{osMajor}, #{osMinor}, " +
//            "   #{browser}, #{browserVersion}, #{browserMajor}, #{deviceBrand}, #{deviceModel}, #{deviceCode}, " +
//            "   #{isBot}, #{platform}, #{loginResult}, #{failReason}, #{responseTime})")
//    void insertLoginLog(UserLoginLog userLoginLog);
}
