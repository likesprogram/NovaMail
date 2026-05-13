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

    // 通过邮箱查询
    @Select("SELECT * FROM MailBoxes mb INNER JOIN Users u ON mb.userId = u.userId WHERE mb.mailBoxId = #{mailBoxId} AND mb.isOpen = 1 LIMIT 1")
    Users selectByMailBoxId(@Param("mailBoxId")Long mailBoxId);

    // 查询账号数量
    @Select("SELECT COUNT(account) FROM Users")
    int getUserCount();

    // 修改用户密码
    @Update("UPDATE Users SET password = #{password} WHERE userId = #{userId}")
    int updatePassword(Users user);

    // 更新登录时间
    @Update("UPDATE Users SET lastLoginTime = CURRENT_TIMESTAMP WHERE userId = #{userId}")
    void updateLastLoginTime(@Param("userId")Long userId);
}
