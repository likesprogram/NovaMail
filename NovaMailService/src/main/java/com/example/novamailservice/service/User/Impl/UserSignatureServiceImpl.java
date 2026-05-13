package com.example.novamailservice.service.User.Impl;

import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.System.SystemSettings;
import com.example.novamailservice.entity.Users.UserSignature;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.mapper.Users.UserSignatureMapper;
import com.example.novamailservice.service.System.SystemSettingsService;
import com.example.novamailservice.service.User.UserSignatureService;
import com.example.novamailservice.util.Cache;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Isolation;
import org.springframework.transaction.annotation.Propagation;
import org.springframework.transaction.annotation.Transactional;

import java.util.ArrayList;
import java.util.List;

@Service
public class UserSignatureServiceImpl implements UserSignatureService {
    private final Logger logger = LoggerFactory.getLogger(UserSignatureServiceImpl.class.getName());
    private final UserSignatureMapper userSignatureMapper;
    private final SystemSettingsService systemSettingsService;
    private final Cache cache;
    private final static String DEFAULT_SIGNATURE_NAME = "我的资料卡";
    private final static String DEFAULT_SIGNATURE_CONTENT = "DefaultSignatureTemplate";
    private static final String UserSignatureCachePrefix = RedisConfig.NovaCache + "UserSignatureCache:";
    private static final String redisKeyPrefixUserId = UserSignatureCachePrefix + "UserId:";
    private static final String redisKeyPrefixSignatureId = UserSignatureCachePrefix + "SignatureId:";
    private static final String redisKeyPrefixDefault = UserSignatureCachePrefix + "Default:UserId:";

    @Autowired
    public UserSignatureServiceImpl(UserSignatureMapper userSignatureMapper, SystemSettingsService systemSettingsService, Cache cache) {
        this.userSignatureMapper = userSignatureMapper;
        this.systemSettingsService = systemSettingsService;
        this.cache = cache;
    }

    @Override
    public ResponseResult<String> createSignature(UserSignature userSignature) {
        try {
            // 冲突判断
            boolean conflict = userSignatureMapper.countSignature(userSignature.getUserId(),userSignature.getSignatureName()) > 0;
            if (conflict) return ResponseResult.error(ErrorCode.CONFLICT, "签名名称不能重复");

            // 创建
            if (userSignatureMapper.createSignature(userSignature) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "签名创建失败");

            // 清理缓存
            cache.removeCache(redisKeyPrefixUserId + userSignature.getUserId());
        } catch (Exception e) {
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "签名创建失败");
        }
        return ResponseResult.success("创建成功");
    }

    @Override
    public boolean createDefaultSignature(Users user) {
        if (user == null) return false;
        UserSignature userSignatures = new UserSignature();
        userSignatures.setUserId(user.getUserId());
        userSignatures.setSignatureName(DEFAULT_SIGNATURE_NAME);
        userSignatures.setSignatureContent(DEFAULT_SIGNATURE_CONTENT);
        userSignatures.setDefault(true);
        return createSignature(userSignatures).isSuccess();
    }

    @Override
    public ResponseResult<List<UserSignature>> getSignatures(Long userId) {
        String userSignatureCacheKey = redisKeyPrefixUserId + userId;
        ResponseResult<List<UserSignature>> cachedResult = cache.getCacheList(userSignatureCacheKey, UserSignature.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;
        try {
            List<UserSignature> userSignatures = userSignatureMapper.getUserSignatures(userId);
            if (userSignatures == null || userSignatures.isEmpty()) return ResponseResult.success(new ArrayList<>());

            for (UserSignature userSignature : userSignatures) { // 可编辑性判断
                if (DEFAULT_SIGNATURE_NAME.equals(userSignature.getSignatureName())) userSignature.setEditable(false);
            }

            // 同步写入缓存
            cache.setCache(userSignatureCacheKey, userSignatures);
            return ResponseResult.success(userSignatures);
        } catch (Exception e) {
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<String> getSignature(Long signatureId) {
        String redisKey = redisKeyPrefixSignatureId + signatureId;
        ResponseResult<String> cachedResult = cache.getCache(redisKey, String.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 查询数据库
        try {
            String signature = userSignatureMapper.getSignatureById(signatureId);
            if (signature == null) return ResponseResult.error(ErrorCode.USER_NOT_FOUND, "签名不存在");

            // 替换系统默认索引
            if (DEFAULT_SIGNATURE_CONTENT.equals(signature)) {
                ResponseResult<SystemSettings> systemDefaultSignature = systemSettingsService.getDefaultSetting();
                if (systemDefaultSignature.isSuccess()) signature = systemDefaultSignature.getData().getSettingValue();
            }

            // 同步写入缓存
            cache.setCache(redisKey, signature);
            return ResponseResult.success(signature);
        } catch (Exception e) {
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<UserSignature> getDefaultSignature(Long userId) {
        String redisKey = redisKeyPrefixDefault + userId;
        ResponseResult<UserSignature> cachedResult = cache.getCache(redisKey, UserSignature.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 查询数据库
        try {
            UserSignature signature = userSignatureMapper.getDefaultSignature(userId);
            if (signature == null) return ResponseResult.error(ErrorCode.USER_NOT_FOUND, "签名不存在");

            // 替换系统默认索引
            if (DEFAULT_SIGNATURE_NAME.equals(signature.getSignatureName())) signature.setEditable(false); // 不可编辑

            // 同步写入缓存
            cache.setCache(redisKey, signature);
            return ResponseResult.success(signature);
        } catch (Exception e) {
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    @Transactional(propagation = Propagation.REQUIRED, isolation = Isolation.DEFAULT)
    public boolean setDefaultSignature(Long signatureId, Long userId, Boolean isDefault) {
        try {
            boolean isSuccess = true;
            cache.removeCache(redisKeyPrefixDefault + userId);
            cache.removeCache(redisKeyPrefixUserId + userId);
            cache.removeCache(redisKeyPrefixSignatureId + signatureId);
            userSignatureMapper.clearDefault(userId);
            if (signatureId > 0 && userSignatureMapper.setDefault(signatureId, userId, isDefault) <= 0) isSuccess = false;
            cache.removeCache(redisKeyPrefixDefault + userId);
            cache.removeCache(redisKeyPrefixUserId + userId);
            cache.removeCache(redisKeyPrefixSignatureId + signatureId);
            return isSuccess;
        } catch (Exception e) {
            throw new RuntimeException("设置默认签名失败");
        }
    }

    @Override
    public boolean updateSignature(UserSignature userSignature) {
        try {
            cache.removeCache(redisKeyPrefixUserId + userSignature.getUserId());
            cache.removeCache(redisKeyPrefixSignatureId + userSignature.getSignatureId());
            if (userSignatureMapper.updateSignature(userSignature) <= 0) return false;
            cache.removeCache(redisKeyPrefixUserId + userSignature.getUserId());
            cache.removeCache(redisKeyPrefixSignatureId + userSignature.getSignatureId());
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    @Override
    public boolean deleteSignature(UserSignature userSignature) {
        try {
            cache.removeCache(redisKeyPrefixUserId + userSignature.getUserId());
            cache.removeCache(redisKeyPrefixSignatureId + userSignature.getSignatureId());
            if (userSignatureMapper.deleteSignature(userSignature.getSignatureId(),userSignature.getUserId()) <= 0) return false;
            cache.removeCache(redisKeyPrefixUserId + userSignature.getUserId());
            cache.removeCache(redisKeyPrefixSignatureId + userSignature.getSignatureId());
            return true;
        } catch (Exception e) {
            logger.error("签名删除失败", e);
            return false;
        }
    }

}
