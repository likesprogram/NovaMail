package com.example.novamailservice.mapper.Users;

import com.example.novamailservice.entity.Users.UserProfile;
import org.apache.ibatis.annotations.*;

@Mapper
public interface UserProfileMapper {
    // 新建用户信息
    @Insert("INSERT INTO UserProfile(userId, nickname, address, defaultMailBoxId) " +
            "VALUES(#{userId}, #{nickname}, #{address}, #{defaultMailBoxId})")
    int createUserProfile(UserProfile userProfile);

    // 查询用户信息
    @Select("SELECT up.* FROM UserProfile up " +
            "INNER JOIN Users u ON up.userId = u.userId " +
            "WHERE u.account = #{account} LIMIT 1")
    UserProfile getUserProfile(@Param("account")String account);

    // 修改昵称
    @Update("UPDATE UserProfile SET nickname = #{nickname} WHERE userId = #{userId}")
    int updateNickname(@Param("userId") Long userId, @Param("nickname") String nickname);

    // 修改头像
    @Update("UPDATE UserProfile SET avatar = #{avatar} WHERE userId = #{userId}")
    int updateUserAvatar(@Param("userId") Long userId, @Param("avatar") Integer avatar);

    // 修改简介
    @Update("UPDATE UserProfile SET resume = #{resume} WHERE userId = #{userId}")
    int updateResume(@Param("userId") Long userId, @Param("resume") String resume);

    // 修改默认邮箱
    @Update("UPDATE UserProfile SET defaultMailBoxId = #{mailBoxId} WHERE userId = #{userId}")
    int updateDefaultMailBox(@Param("userId") Long userId, @Param("mailBoxId") Long mailBoxId);
}
