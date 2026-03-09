package com.example.novamailservice.service.UserProfile;

import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.mapper.Users.UsersMapper;
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
    private static final String redisKeyPrefixUserId = UsersCachePrefix + "UserId:";
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
    public ResponseResult<Users> getUserById(Integer userId) {
        String redisKey = redisKeyPrefixUserId + userId;
        ResponseResult<Users> cachedResult = cache.getCache(redisKey, Users.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 查询数据库
        try {
            Users user = usersMapper.selectById(userId);
            if (user == null) return ResponseResult.error(ErrorCode.USER_NOT_FOUND, "用户不存在");

            // 同步写入缓存
            cache.setCache(redisKey, user);
            return ResponseResult.success(user);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }
}
