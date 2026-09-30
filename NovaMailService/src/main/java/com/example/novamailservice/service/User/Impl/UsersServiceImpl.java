package com.example.novamailservice.service.User.Impl;

import com.example.novamailservice.config.Filter.RequestContext;
import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.UpdatePassword;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.mapper.Users.UsersMapper;
import com.example.novamailservice.service.User.UsersService;
import com.example.novamailservice.util.Cache;
import com.example.novamailservice.util.JwtTokenProvider;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Service
public class UsersServiceImpl implements UsersService {
    private final Logger logger = LoggerFactory.getLogger(UsersServiceImpl.class.getName());
    private final JwtTokenProvider jwtTokenProvider;
    private final UsersMapper usersMapper;
    private final Cache cache;
    private static final String UsersCachePrefix = RedisConfig.NovaCache + "UsersCache:";
    private static final String redisKeyPrefixAccount = UsersCachePrefix + "Account:";
    private static final String redisKeyPrefixMailBoxId = UsersCachePrefix + "MailBoxId:";

    @Autowired
    public UsersServiceImpl(JwtTokenProvider jwtTokenProvider, UsersMapper usersMapper, Cache cache) {
        this.jwtTokenProvider = jwtTokenProvider;
        this.usersMapper = usersMapper;
        this.cache = cache;
    }
    @Override
    public boolean createUser(Users user) {
        // 密码加密
        String passwordHash = jwtTokenProvider.encodePassword(user.getPassword());
        user.setPassword(passwordHash);
        return usersMapper.register(user) > 0;
    }

    @Override
    public ResponseResult<Users> getCurrentUser() {
        String account = RequestContext.getCurrentAccount();
        return getUserByAccount(account);
    }

    @Override
    public ResponseResult<Users> getUserByAccount(String account) {
        String redisKey = redisKeyPrefixAccount + account;
        ResponseResult<Users> cachedResult = cache.getCache(redisKey, Users.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 查询数据库
        try {
            Users user = usersMapper.selectByAccount(account);
            if (user == null) return ResponseResult.error(ErrorCode.USER_NOT_FOUND, "用户不存在");

            // 同步写入缓存
            cache.setCache(redisKey, user);
            return ResponseResult.success(user);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<Users> getUserByMailBoxId(Long mailBoxId) {
        String redisKey = redisKeyPrefixMailBoxId + mailBoxId;
        ResponseResult<Users> cachedResult = cache.getCache(redisKey, Users.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 查询数据库
        try {
            Users user = usersMapper.selectByMailBoxId(mailBoxId);
            if (user == null) return ResponseResult.error(ErrorCode.USER_NOT_FOUND, "用户不存在");

            // 同步写入缓存
            cache.setCache(redisKey, user);
            return ResponseResult.success(user);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public void updateLastLoginTime(Long userId, String account) {
        try {
            // 移除缓存
            cache.removeCache(redisKeyPrefixAccount + account);
            // 更新数据
            usersMapper.updateLastLoginTime(userId);
            // 移除缓存
            cache.removeCache(redisKeyPrefixAccount + account);
        } catch (Exception e) {
            logger.error(e.getMessage());
        }
    }

    @Override
    public ResponseResult<String> updatePassword(Users user, UpdatePassword updatePassword) {
        try {
            // 确认密码与新密码不一致
            if (!updatePassword.getNewPassword().equals(updatePassword.getConfirmPassword())) return ResponseResult.error(ErrorCode.USER_INVALID, "新密码与确认密码不一致");

            // 旧密码校验
            if (!jwtTokenProvider.matchesPassword(updatePassword.getOldPassword(), user.getPassword())) return ResponseResult.error(ErrorCode.USER_PASSWORD_ERROR, "旧密码错误");

            // 新密码与旧密码一致
            if (updatePassword.getOldPassword().equals(updatePassword.getConfirmPassword())) return ResponseResult.error(ErrorCode.CONFLICT, "新密码不能与旧密码一致");

            // 密码安全性校验
            ResponseResult<String> validatePassword = Users.validatePassword(updatePassword.getConfirmPassword());
            if (!validatePassword.isSuccess()) return ResponseResult.error(ErrorCode.USER_INVALID, validatePassword.getMessage());

            // 密码加密
            user.setPassword(jwtTokenProvider.encodePassword(updatePassword.getConfirmPassword()));

            // 移除缓存
            cache.removeCache(redisKeyPrefixAccount + user.getAccount());
            // 更新密码
            if (usersMapper.updatePassword(user) > 0) {
                cache.removeCache(redisKeyPrefixAccount + user.getAccount());
                return ResponseResult.success("修改成功");
            }
            return ResponseResult.error(ErrorCode.DATABASE_ERROR,"修改失败");
        } catch (Exception e) {
            logger.error(e.getMessage());
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "密码修改失败");
        }
    }

    @Override
    public ResponseResult<String> resetPassword(Users user) {
        try {
            if (user == null || user.getUserId() == null) return ResponseResult.error(ErrorCode.USER_INVALID, "用户信息无效");

            cache.removeCache(redisKeyPrefixAccount + user.getAccount());
            if (usersMapper.updatePassword(user) > 0) {
                cache.removeCache(redisKeyPrefixAccount + user.getAccount());
                return ResponseResult.success("重置成功");
            }

            return ResponseResult.error(ErrorCode.DATABASE_ERROR,"重置失败");
        } catch (Exception e) {
            logger.error(e.getMessage());
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "密码重置失败");
        }
    }
}
