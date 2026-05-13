package com.example.novamailservice.service.DomainNames.Impl;

import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.DomainNames.DomainNames;
import com.example.novamailservice.mapper.DomainNames.DomainNamesMapper;
import com.example.novamailservice.service.DomainNames.DomainNamesService;
import com.example.novamailservice.util.Cache;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class DomainNamesServiceImpl implements DomainNamesService {
    private final Logger logger = LoggerFactory.getLogger(DomainNamesServiceImpl.class.getName());
    private final DomainNamesMapper domainNamesMapper;
    private final Cache cache;
    private static final String DomainNamesCachePrefix = RedisConfig.NovaCache + "DomainNamesCache:";
    private static final String redisKeyPrefixDefaultDomainName = DomainNamesCachePrefix + "DefaultDomainName";
    private static final String redisKeyPrefixDefaultDomainNameById = DomainNamesCachePrefix + "Id:";
    private static final String redisKeyPrefixDefaultDomainNameByName = DomainNamesCachePrefix + "Name:";
    private static final String redisKeyPrefixDefaultDomainNameByUserId = DomainNamesCachePrefix + "UserId:";
    @Autowired
    public DomainNamesServiceImpl(DomainNamesMapper domainNamesMapper, Cache cache) {
        this.domainNamesMapper = domainNamesMapper;
        this.cache = cache;
    }

    @Override // 获取系统默认域名
    public ResponseResult<DomainNames> getSystemDefaultDomainName() {
        // Redis 缓存查询
        ResponseResult<DomainNames> cachedResult = cache.getCache(redisKeyPrefixDefaultDomainName, DomainNames.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            DomainNames defaultDomainName = domainNamesMapper.getSystemDefaultDomainName();
            if (defaultDomainName != null) {
                cache.setCache(redisKeyPrefixDefaultDomainName, defaultDomainName);
                return ResponseResult.success(defaultDomainName);
            }
            return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "系统默认域名不存在");
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<DomainNames> getDomainNameById(Long domainNameId) {
        if (domainNameId == null || domainNameId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // Redis 缓存查询
        String redisKey = redisKeyPrefixDefaultDomainNameById + domainNameId;
        ResponseResult<DomainNames> cachedResult = cache.getCache(redisKey, DomainNames.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            DomainNames existing = domainNamesMapper.selectById(domainNameId);
            if (existing == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");

            // 同步写入缓存
            cache.setCache(redisKey, existing);
            return ResponseResult.success(existing);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<DomainNames> getDomainNameByName(String domainName) {
        if (domainName == null || domainName.isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // Redis 缓存查询
        String redisKey = redisKeyPrefixDefaultDomainNameByName + domainName;
        ResponseResult<DomainNames> cachedResult = cache.getCache(redisKey, DomainNames.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            DomainNames existing = domainNamesMapper.selectByName(domainName);
            if (existing == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");

            // 同步写入缓存
            cache.setCache(redisKey, existing);
            return ResponseResult.success(existing);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<List<DomainNames>> getDomainNameByUserId(Long userId) {
        if (userId == null || userId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // Redis 缓存查询
        String redisKey = redisKeyPrefixDefaultDomainNameByUserId + userId;
        ResponseResult<List<DomainNames>> cachedResult = cache.getCacheList(redisKey, DomainNames.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            List<DomainNames> domainNamesList = domainNamesMapper.selectByUserId(userId);
            if (domainNamesList == null || domainNamesList.isEmpty()) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "该用户没有域名");

            // 同步写入缓存
            cache.setCache(redisKey, domainNamesList);
            return ResponseResult.success(domainNamesList);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }


}
