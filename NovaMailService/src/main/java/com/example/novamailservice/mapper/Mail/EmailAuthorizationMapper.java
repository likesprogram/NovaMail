package com.example.novamailservice.mapper.Mail;

import com.example.novamailservice.entity.Mail.EmailAuthorization;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface EmailAuthorizationMapper {
    @Select("SELECT * FROM EmailAuthorization WHERE userId = #{userId} ORDER BY updateTime DESC, emailAuthorizationId DESC")
    List<EmailAuthorization> selectByUserId(@Param("userId") Long userId);

    @Select("SELECT * FROM EmailAuthorization WHERE emailAuthorizationId = #{emailAuthorizationId} AND userId = #{userId} LIMIT 1")
    EmailAuthorization selectByIdAndUserId(@Param("emailAuthorizationId") Long emailAuthorizationId, @Param("userId") Long userId);

    @Select("SELECT * FROM EmailAuthorization WHERE authCode = #{authCode} LIMIT 1")
    EmailAuthorization selectByAuthCode(@Param("authCode") String authCode);

    @Insert("""
            INSERT INTO EmailAuthorization(userId, authCode, expireTime, isActivated, clientInfo)
            VALUES(#{userId}, #{authCode}, #{expireTime}, #{isActivated}, #{clientInfo})
            """)
    @Options(useGeneratedKeys = true, keyProperty = "emailAuthorizationId")
    int createEmailAuthorization(EmailAuthorization emailAuthorization);

    @Update("""
            UPDATE EmailAuthorization
            SET expireTime = #{expireTime},
                isActivated = #{isActivated},
                clientInfo = #{clientInfo}
            WHERE emailAuthorizationId = #{emailAuthorizationId}
              AND userId = #{userId}
            """)
    int updateEmailAuthorization(EmailAuthorization emailAuthorization);

    @Update("""
            UPDATE EmailAuthorization
            SET isActivated = #{isActivated}
            WHERE emailAuthorizationId = #{emailAuthorizationId}
              AND userId = #{userId}
            """)
    int updateActivated(@Param("emailAuthorizationId") Long emailAuthorizationId,
                        @Param("userId") Long userId,
                        @Param("isActivated") Integer isActivated);

    @Delete("DELETE FROM EmailAuthorization WHERE emailAuthorizationId = #{emailAuthorizationId} AND userId = #{userId}")
    int deleteByIdAndUserId(@Param("emailAuthorizationId") Long emailAuthorizationId, @Param("userId") Long userId);
}
