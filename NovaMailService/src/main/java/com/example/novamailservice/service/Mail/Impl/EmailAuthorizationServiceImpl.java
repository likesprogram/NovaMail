package com.example.novamailservice.service.Mail.Impl;

import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Mail.EmailAuthorization;
import com.example.novamailservice.mapper.Mail.EmailAuthorizationMapper;
import com.example.novamailservice.service.Mail.EmailAuthorizationService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.security.SecureRandom;
import java.time.LocalDateTime;
import java.util.List;

@Service
public class EmailAuthorizationServiceImpl implements EmailAuthorizationService {
    private final Logger logger = LoggerFactory.getLogger(EmailAuthorizationServiceImpl.class.getName());
    private static final SecureRandom SECURE_RANDOM = new SecureRandom();
    private static final char[] CODE_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789".toCharArray();

    private final EmailAuthorizationMapper emailAuthorizationMapper;

    @Autowired
    public EmailAuthorizationServiceImpl(EmailAuthorizationMapper emailAuthorizationMapper) {
        this.emailAuthorizationMapper = emailAuthorizationMapper;
    }

    @Override
    public ResponseResult<List<EmailAuthorization>> getAuthorizationList(Long userId) {
        if (userId == null || userId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        try {
            return ResponseResult.success(emailAuthorizationMapper.selectByUserId(userId));
        } catch (Exception e) {
            logger.error("SMTP/IMAP 授权列表获取失败: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "授权列表获取失败");
        }
    }

    @Override
    public ResponseResult<EmailAuthorization> createAuthorization(Long userId, EmailAuthorization emailAuthorization) {
        if (userId == null || userId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        ResponseResult<String> validation = validateAuthorization(emailAuthorization);
        if (!validation.isSuccess()) return ResponseResult.error(validation.getCode(), validation.getMessage());

        try {
            emailAuthorization.setUserId(userId);
            emailAuthorization.setAuthCode(generateUniqueAuthCode());
            emailAuthorization.setIsActivated(Integer.valueOf(0).equals(emailAuthorization.getIsActivated()) ? 0 : 1);
            emailAuthorization.setClientInfo(normalizeClientInfo(emailAuthorization.getClientInfo()));

            int rows = emailAuthorizationMapper.createEmailAuthorization(emailAuthorization);
            if (rows <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "授权码创建失败");

            EmailAuthorization created = emailAuthorizationMapper.selectByIdAndUserId(emailAuthorization.getEmailAuthorizationId(), userId);
            return ResponseResult.success(created == null ? emailAuthorization : created);
        } catch (Exception e) {
            logger.error("SMTP/IMAP 授权创建失败: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "授权码创建失败");
        }
    }

    @Override
    public ResponseResult<EmailAuthorization> updateAuthorization(Long userId, EmailAuthorization emailAuthorization) {
        if (userId == null || userId <= 0 || emailAuthorization == null || emailAuthorization.getEmailAuthorizationId() == null) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }
        ResponseResult<String> validation = validateAuthorization(emailAuthorization);
        if (!validation.isSuccess()) return ResponseResult.error(validation.getCode(), validation.getMessage());

        try {
            EmailAuthorization existing = emailAuthorizationMapper.selectByIdAndUserId(emailAuthorization.getEmailAuthorizationId(), userId);
            if (existing == null) return ResponseResult.error(ErrorCode.NOT_FOUND, "授权码不存在");

            emailAuthorization.setUserId(userId);
            emailAuthorization.setIsActivated(Integer.valueOf(0).equals(emailAuthorization.getIsActivated()) ? 0 : 1);
            emailAuthorization.setClientInfo(normalizeClientInfo(emailAuthorization.getClientInfo()));

            if (emailAuthorizationMapper.updateEmailAuthorization(emailAuthorization) <= 0) {
                return ResponseResult.error(ErrorCode.DATABASE_ERROR, "授权码修改失败");
            }

            return ResponseResult.success(emailAuthorizationMapper.selectByIdAndUserId(emailAuthorization.getEmailAuthorizationId(), userId));
        } catch (Exception e) {
            logger.error("SMTP/IMAP 授权修改失败: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "授权码修改失败");
        }
    }

    @Override
    public ResponseResult<String> updateAuthorizationActivated(Long userId, Long emailAuthorizationId, Integer isActivated) {
        if (userId == null || userId <= 0 || emailAuthorizationId == null || emailAuthorizationId <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }
        if (isActivated == null || (isActivated != 0 && isActivated != 1)) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        try {
            if (emailAuthorizationMapper.updateActivated(emailAuthorizationId, userId, isActivated) <= 0) {
                return ResponseResult.error(ErrorCode.NOT_FOUND, "授权码不存在");
            }
            return ResponseResult.success("操作成功");
        } catch (Exception e) {
            logger.error("SMTP/IMAP 授权状态更新失败: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "授权码状态更新失败");
        }
    }

    @Override
    public ResponseResult<String> deleteAuthorization(Long userId, Long emailAuthorizationId) {
        if (userId == null || userId <= 0 || emailAuthorizationId == null || emailAuthorizationId <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        try {
            if (emailAuthorizationMapper.deleteByIdAndUserId(emailAuthorizationId, userId) <= 0) {
                return ResponseResult.error(ErrorCode.NOT_FOUND, "授权码不存在");
            }
            return ResponseResult.success("操作成功");
        } catch (Exception e) {
            logger.error("SMTP/IMAP 授权删除失败: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "授权码删除失败");
        }
    }

    private ResponseResult<String> validateAuthorization(EmailAuthorization emailAuthorization) {
        if (emailAuthorization == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        String clientInfo = emailAuthorization.getClientInfo();
        if (clientInfo == null || clientInfo.trim().isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "请输入授权名称");
        if (clientInfo.trim().length() > 255) return ResponseResult.error(ErrorCode.PARAM_INVALID, "授权名称不能超过255个字符");
        LocalDateTime expireTime = emailAuthorization.getExpireTime();
        if (expireTime != null && !expireTime.isAfter(LocalDateTime.now())) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "过期时间必须晚于当前时间");
        }
        return ResponseResult.success();
    }

    private String normalizeClientInfo(String clientInfo) {
        if (clientInfo == null || clientInfo.trim().isEmpty()) return "SMTP/IMAP 客户端";
        return clientInfo.trim();
    }

    private String generateUniqueAuthCode() {
        for (int i = 0; i < 10; i++) {
            String authCode = generateAuthCode();
            if (emailAuthorizationMapper.selectByAuthCode(authCode) == null) return authCode;
        }
        throw new IllegalStateException("无法生成唯一授权码");
    }

    private String generateAuthCode() {
        StringBuilder builder = new StringBuilder("nova-");
        for (int group = 0; group < 4; group++) {
            if (group > 0) builder.append('-');
            for (int i = 0; i < 6; i++) {
                builder.append(CODE_ALPHABET[SECURE_RANDOM.nextInt(CODE_ALPHABET.length)]);
            }
        }
        return builder.toString();
    }
}
