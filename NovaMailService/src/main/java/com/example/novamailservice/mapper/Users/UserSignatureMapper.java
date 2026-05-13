package com.example.novamailservice.mapper.Users;

import com.example.novamailservice.entity.Users.UserSignature;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface UserSignatureMapper {
    // 新建用户签名
    @Insert("INSERT INTO UserSignatures(userId, signatureName, signatureContent, isDefault) " +
            "VALUES(#{userId}, #{signatureName}, #{signatureContent}, #{isDefault})")
    int createSignature(UserSignature userSignature);

    // 是否存在
    @Select("SELECT COUNT(signatureId) FROM UserSignatures " +
            "WHERE userId = #{userId} AND signatureName = #{signatureName}")
    int countSignature(@Param("userId") Long userId, @Param("signatureName") String signatureName);

    // 获取用户所有签名
    @Select("SELECT signatureId, userId, signatureName, isDefault, createTime, updateTime " +
            "FROM UserSignatures WHERE userId = #{userId}")
    List<UserSignature> getUserSignatures(@Param("userId") Long userId);

    // 获取签名内容
    @Select("SELECT signatureContent FROM UserSignatures WHERE SignatureId = #{SignatureId} LIMIT 1")
    String getSignatureById(@Param("SignatureId") Long SignatureId);

    // 获取默认签名
    @Select("SELECT signatureId, userId, signatureName, isDefault, createTime, updateTime " +
            "FROM UserSignatures WHERE userId = #{userId} AND isDefault = 1 LIMIT 1")
    UserSignature getDefaultSignature(@Param("userId") Long userId);

    // 清除默认签名
    @Update("UPDATE UserSignatures SET isDefault = 0 WHERE userId = #{userId}")
    void clearDefault(@Param("userId") Long userId);

    // 设置默认签名
    @Update("UPDATE UserSignatures SET isDefault = #{isDefault} WHERE userId = #{userId} AND signatureId = #{signatureId}")
    int setDefault(@Param("signatureId") Long signatureId, @Param("userId") Long userId, @Param("isDefault") Boolean isDefault);

    // 修改签名
    @Update("UPDATE UserSignatures SET signatureName = #{signatureName}, signatureContent = #{signatureContent} " +
            "WHERE signatureId = #{signatureId} AND userId = #{userId}")
    int updateSignature(UserSignature userSignature);

    // 删除签名
    @Delete("DELETE FROM UserSignatures WHERE signatureId = #{signatureId} AND userId = #{userId}")
    int deleteSignature(@Param("signatureId") Long signatureId, @Param("userId") Long userId);
}
