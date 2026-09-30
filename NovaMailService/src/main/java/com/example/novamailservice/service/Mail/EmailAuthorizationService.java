package com.example.novamailservice.service.Mail;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Mail.EmailAuthorization;

import java.util.List;

public interface EmailAuthorizationService {
    ResponseResult<List<EmailAuthorization>> getAuthorizationList(Long userId);

    ResponseResult<EmailAuthorization> createAuthorization(Long userId, EmailAuthorization emailAuthorization);

    ResponseResult<EmailAuthorization> updateAuthorization(Long userId, EmailAuthorization emailAuthorization);

    ResponseResult<String> updateAuthorizationActivated(Long userId, Long emailAuthorizationId, Integer isActivated);

    ResponseResult<String> deleteAuthorization(Long userId, Long emailAuthorizationId);
}
