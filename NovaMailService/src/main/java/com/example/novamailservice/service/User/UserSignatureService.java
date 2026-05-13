package com.example.novamailservice.service.User;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Users.UserSignature;
import com.example.novamailservice.entity.Users.Users;

import java.util.List;

public interface UserSignatureService {
    // 创建签名
    ResponseResult<String> createSignature(UserSignature UserSignature);
    boolean createDefaultSignature(Users user);

    // 获取签名列表
    ResponseResult<List<UserSignature>> getSignatures(Long userId);

    // 获取签名内容
    ResponseResult<String> getSignature(Long signatureId);

    // 获取默认签名
    ResponseResult<UserSignature> getDefaultSignature(Long userId);

    // 设置默认签名
    boolean setDefaultSignature(Long signatureId, Long userId, Boolean isDefault);

    // 更新签名
    boolean updateSignature(UserSignature UserSignature);

    // 删除签名
    boolean deleteSignature(UserSignature UserSignature);
}
