package com.example.novamailservice.mapper.Users;

import com.example.novamailservice.entity.Users.UserProfile;
import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Mapper;

@Mapper
public interface UserProfileMapper {
    // 新建用户信息
    @Insert("INSERT INTO UserProfile(userId, nickname, avatar, address, resume, defaultMailBoxId) " +
            "VALUES(#{userId}, #{nickname}, #{avatar}, #{address}, #{resume}, #{defaultMailBoxId})")
    int createUserProfile(UserProfile userProfile);
}
