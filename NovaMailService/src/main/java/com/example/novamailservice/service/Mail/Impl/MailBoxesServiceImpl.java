package com.example.novamailservice.service.Mail.Impl;

import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.MailBoxAddressToMailBoxAndUser;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Result.ServiceException;
import com.example.novamailservice.entity.DomainNames.DomainNames;
import com.example.novamailservice.entity.Mail.MailBoxChangeLog;
import com.example.novamailservice.entity.Mail.MailBoxes;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.mapper.Mail.MailBoxesMapper;
import com.example.novamailservice.service.DomainNames.DomainNamesService;
import com.example.novamailservice.service.Mail.MailBoxChangeLogService;
import com.example.novamailservice.service.Mail.MailBoxFolderStateService;
import com.example.novamailservice.service.Mail.MailBoxesService;
import com.example.novamailservice.service.User.UsersService;
import com.example.novamailservice.util.Cache;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.List;
import org.slf4j.Logger;
import org.springframework.transaction.annotation.Transactional;

@Service
public class MailBoxesServiceImpl implements MailBoxesService {
    private final Logger logger = LoggerFactory.getLogger(MailBoxesServiceImpl.class.getName());
    private final DomainNamesService domainNamesService;
    private final MailBoxesMapper mailBoxesMapper;
    private final UsersService usersService;
    private final MailBoxChangeLogService mailBoxChangeLogService;
    private final MailBoxFolderStateService mailBoxFolderStateService;
    private final Cache cache;

    private static final String MailBoxCachePrefix = RedisConfig.NovaCache + "MailBoxCache:";
    private static final String redisKeyPrefixDomainNameIdAndMailBoxName = MailBoxCachePrefix + "DomainNameIdMailBoxId:";
    private static final String redisKeyPrefixMailBoxId = MailBoxCachePrefix + "MailBoxId:";
    private static final String redisKeyPrefixMailBoxAddressId = MailBoxCachePrefix + "MailBoxAddressById:";
    private static final String redisKeyPrefixUserId = MailBoxCachePrefix + "UserId:";
    private static final String redisKeyPrefixDomainNameId = MailBoxCachePrefix + "DomainNameId:";
    private static final String redisKeyPrefixMailBoxAddress = MailBoxCachePrefix + "MailBoxAddress:";

    @Autowired
    public MailBoxesServiceImpl(DomainNamesService domainNamesService, MailBoxesMapper mailBoxesMapper, UsersService usersService, MailBoxChangeLogService mailBoxChangeLogService, MailBoxFolderStateService mailBoxFolderStateService, Cache cache) {
        this.domainNamesService = domainNamesService;
        this.mailBoxesMapper = mailBoxesMapper;
        this.usersService = usersService;
        this.mailBoxChangeLogService = mailBoxChangeLogService;
        this.mailBoxFolderStateService = mailBoxFolderStateService;
        this.cache = cache;
    }

    @Override
    @Transactional
    public ResponseResult<MailBoxes> createMailBox(Users user, DomainNames domainName, String mailBoxName, int retry) {
        if (user == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        DomainNames defaultDomainName = domainName;

        if (defaultDomainName == null) {
            if (retry > 0) {
                logger.info("用户：[{}] 创建邮箱, 正在尝试更换默认域名...", user.getAccount());
                ResponseResult<DomainNames> defaultDomainNameResult = domainNamesService.getSystemDefaultDomainName();
                if (defaultDomainNameResult.isSuccess()) defaultDomainName = defaultDomainNameResult.getData();
            } else {
                logger.error("用户：[{}] 创建邮箱失败：找不到有效域名", user.getAccount());
                return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");
            }
            if (defaultDomainName == null)  {
                logger.error("用户：[{}] 创建邮箱失败：找不到有效域名", user.getAccount());
                return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");
            }
        }

        // 邮箱本地部分（即 @ 前面的部分）
        String localPart = (mailBoxName == null || mailBoxName.isEmpty())
                ? user.getAccount()
                : mailBoxName;

        // 邮箱名称校验
        ResponseResult<String> validateMailBoxName = MailBoxes.validateMailBoxAddress(localPart);
        if (!validateMailBoxName.isSuccess()) return ResponseResult.error(ErrorCode.MAILBOX_INVALID, validateMailBoxName.getMessage());

        // 邮箱名称唯一性检查
        String baseName = localPart.trim();
        String candidate = baseName;
        ResponseResult<MailBoxes> mailBox = getMailBoxByName(defaultDomainName.getDomainNameId(), baseName);
        if (mailBox.isSuccess()) {
            if (retry <= 0) {
                logger.error("用户：[{}] 创建邮箱失败：邮箱名 [{}] 已被占用", user.getAccount(), baseName);
                return ResponseResult.error(ErrorCode.MAILBOX_EXISTS, "邮箱名称已被占用");
            }
            logger.info("邮箱名 [{}] 已存在，尝试生成唯一名称", baseName);
            // 邮箱已存在，尝试生成新的邮箱名称
            for (int attempt = 0; attempt < retry; attempt++) {
                candidate = (attempt == 0) ? baseName : baseName + attempt;
                mailBox = getMailBoxByName(defaultDomainName.getDomainNameId(), candidate);
                if (!mailBox.isSuccess()) break;

                if (attempt == retry - 1) {
                    logger.error("用户：[{}] 邮箱创建失败：无法生成唯一邮箱名 (base={})", user.getAccount(), baseName);
                    return ResponseResult.error(ErrorCode.MAILBOX_UNIQUE_FAIL, "无法生成唯一邮箱名，请稍后重试");
                }
            }
        }

        // 创建邮箱
        MailBoxes newMailBox = new MailBoxes();
        newMailBox.setUserId(user.getUserId());
        newMailBox.setDomainNameId(defaultDomainName.getDomainNameId());
        newMailBox.setLocalPart(candidate);
        newMailBox.setIsOpen(1); // 启用
        newMailBox.setCreateTime(java.time.LocalDateTime.now());
        newMailBox.setUpdateTime(newMailBox.getCreateTime());

        int rows = mailBoxesMapper.createMailBox(newMailBox);
        if (rows <= 0) {
            logger.error("用户：[{}] 创建邮箱 [{}] 失败", user.getAccount(), candidate);
            return ResponseResult.error(ErrorCode.MAILBOX_CREATE_FAIL, "创建邮箱失败");
        }

        // 初始化该邮箱的系统文件夹 IMAP UID 状态
        boolean stateCreated = mailBoxFolderStateService.initSystemFolderStates(newMailBox.getMailBoxId());
        if (!stateCreated) throw new ServiceException(ErrorCode.DATABASE_ERROR, "初始化邮箱文件夹状态失败");

        // 写入日志
        MailBoxChangeLog log = new MailBoxChangeLog();
        log.setMailBoxId(newMailBox.getMailBoxId());
        log.setUserId(user.getUserId());
        log.setAction("CREATE");
        log.setOldValue("{}"); // 没有旧值
        try {
            ObjectMapper objectMapper = new ObjectMapper();
            log.setNewValue(objectMapper.writeValueAsString(newMailBox)); // 临时序列化
        } catch (Exception e) {
            log.setNewValue("{}"); // 序列化失败 给默认值
        }

        log.setRemark("注册账号，创建默认邮箱");
        mailBoxChangeLogService.writeLog(log);
        return ResponseResult.success(newMailBox);
    }

    @Override
    public ResponseResult<MailBoxes> getMailBoxByName(Long domainNameId, String mailBoxName) {
        if (domainNameId == null || domainNameId <= 0 || mailBoxName == null || mailBoxName.isEmpty())
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // Redis 查询
        String redisKey = redisKeyPrefixDomainNameIdAndMailBoxName + domainNameId + ":" + mailBoxName;
        ResponseResult<MailBoxes> cachedResult = cache.getCache(redisKey, MailBoxes.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            ResponseResult<DomainNames> domainName = domainNamesService.getDomainNameById(domainNameId);
            if (!domainName.isSuccess()) return ResponseResult.error(domainName.getCode(), domainName.getMessage());
            MailBoxes existing = mailBoxesMapper.selectByDomainAndAddress(domainNameId, mailBoxName);
            if (existing == null) {
                logger.warn("邮箱不存在: {}@{}", mailBoxName, domainName.getData().getDomainName());
                return ResponseResult.error(ErrorCode.MAILBOX_NOT_FOUND, "邮箱不存在");
            }

            // 同步写入缓存
            cache.setCache(redisKey, existing);
            return ResponseResult.success(existing);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<MailBoxes> getMailBoxById(Long mailBoxId) {
        if (mailBoxId == null || mailBoxId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // Redis 查询
        String redisKey = redisKeyPrefixMailBoxId + mailBoxId;
        ResponseResult<MailBoxes> cachedResult = cache.getCache(redisKey, MailBoxes.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            MailBoxes existing = mailBoxesMapper.selectById(mailBoxId);
            if (existing == null) return ResponseResult.error(ErrorCode.MAILBOX_NOT_FOUND, "邮箱不存在");

            // 同步写入缓存
            cache.setCache(redisKey, existing);
            return ResponseResult.success(existing);
         } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<List<MailBoxes>> getMailBoxListByUserId(Long userId) {
        if (userId == null || userId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误 ");

        // Redis 查询
        String redisKey = redisKeyPrefixUserId + userId;
        ResponseResult<List<MailBoxes>> cachedResult = cache.getCacheList(redisKey, MailBoxes.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            List<MailBoxes> mailBoxList = mailBoxesMapper.selectByUserId(userId);
            if (mailBoxList == null || mailBoxList.isEmpty()) return ResponseResult.error(ErrorCode.MAILBOX_NOT_FOUND, "该用户没有邮箱");

            // 同步写入缓存
            cache.setCache(redisKey, mailBoxList);
            return ResponseResult.success(mailBoxList);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<List<MailBoxes>> getMailBoxListByDomainNameId(Long domainNameId) {
        if (domainNameId == null || domainNameId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // Redis 查询
        String redisKey = redisKeyPrefixDomainNameId + domainNameId;
        ResponseResult<List<MailBoxes>> cachedResult = cache.getCacheList(redisKey, MailBoxes.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            List<MailBoxes> mailBoxList = mailBoxesMapper.selectByDomainId(domainNameId);
            if (mailBoxList == null || mailBoxList.isEmpty()) return ResponseResult.error(ErrorCode.MAILBOX_NOT_FOUND, "该域名没有邮箱");

            // 同步写入缓存
            cache.setCache(redisKey, mailBoxList);
            return ResponseResult.success(mailBoxList);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<List<MailBoxes>> getAllMailBoxes() {
        return null;
    }

    // 辅助函数：从数据库获取邮箱地址
    private ResponseResult<String> GetMailBoxAddressDatabase(MailBoxes mailBox) {
        if (mailBox == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // 数据库查询
        try {
            ResponseResult<DomainNames> domain = domainNamesService.getDomainNameById(mailBox.getDomainNameId());
            if (!domain.isSuccess()) {
                logger.warn("域名不存在 ID={}", mailBox.getDomainNameId());
                return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "邮箱域名不存在");
            }

            String mailBoxAddress = mailBox.getLocalPart() + "@" + domain.getData().getDomainName();

            // 同步写入缓存
            String redisKey = redisKeyPrefixMailBoxAddressId + mailBox.getMailBoxId();
            cache.setCache(redisKey, mailBoxAddress);
            return ResponseResult.success(mailBoxAddress);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<String> getMailBoxAddress(Long mailBoxId) {
        if (mailBoxId == null || mailBoxId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // 缓存查询
        String redisKey = redisKeyPrefixMailBoxAddressId + mailBoxId;
        ResponseResult<String> cachedResult = cache.getCache(redisKey, String.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询邮箱
        ResponseResult<MailBoxes> mailBoxes = getMailBoxById(mailBoxId);
        if (!mailBoxes.isSuccess()) return ResponseResult.error(ErrorCode.MAILBOX_NOT_FOUND, "邮箱不存在");

        // 查询数据库
        return GetMailBoxAddressDatabase(mailBoxes.getData());
    }

    @Override
    public ResponseResult<String> getMailBoxAddress(MailBoxes mailBox) {
        if (mailBox == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // 缓存查询
        String redisKey = redisKeyPrefixMailBoxAddressId + mailBox.getMailBoxId();
        ResponseResult<String> cachedResult = cache.getCache(redisKey, String.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 查询数据库
        return GetMailBoxAddressDatabase(mailBox);
    }

    @Override
    public ResponseResult<MailBoxAddressToMailBoxAndUser> getAccountByMailBoxAddress(String mailBoxAddress) {
        if (mailBoxAddress == null || mailBoxAddress.isBlank()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱地址不能为空");
        String address = mailBoxAddress.trim();
        if (address.length() > 320) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱地址过长");
        // 当前只接受纯邮箱地址，不接受：张三 <xxx@domain.com>
        if (address.contains("<") || address.contains(">") || address.contains(" ")) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱地址格式错误");
        int atIndex = address.indexOf('@');
        if (atIndex <= 0 || atIndex != address.lastIndexOf('@') || atIndex >= address.length() - 1) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱地址格式错误");
        String localPart = address.substring(0, atIndex);
        String domainName = address.substring(atIndex + 1).toLowerCase();
        // 校验 localPart
        if (localPart.isBlank()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱本地部分不能为空");
        if (localPart.length() > 255) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱本地部分过长");
        if (localPart.startsWith(".") || localPart.endsWith(".")) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱本地部分不能以点号开头或结尾");
        if (localPart.contains("..")) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱本地部分不能包含连续点号");
        if (!localPart.matches("^[A-Za-z0-9._%+-]+$")) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱本地部分包含非法字符");
        // 校验 domainName
        if (domainName.isBlank()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱域名不能为空");
        if (domainName.length() > 253) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱域名过长");
        if (domainName.startsWith(".") || domainName.endsWith(".")) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱域名不能以点号开头或结尾");
        if (domainName.contains("..")) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱域名不能包含连续点号");
        String[] domainParts = domainName.split("\\.");
        if (domainParts.length < 2) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱域名格式错误");
        for (String part : domainParts) {
            if (part.isBlank()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱域名格式错误");
            if (part.length() > 63) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱域名单段过长");
            if (!part.matches("^[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?$")) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱域名包含非法字符");
        }

        // 缓存查询
        String redisKey = redisKeyPrefixMailBoxAddress + mailBoxAddress;
        ResponseResult<MailBoxAddressToMailBoxAndUser> cachedResult = cache.getCache(redisKey, MailBoxAddressToMailBoxAndUser.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            MailBoxAddressToMailBoxAndUser result = new MailBoxAddressToMailBoxAndUser();
            ResponseResult<DomainNames> domain = domainNamesService.getDomainNameByName(domainName);
            if (!domain.isSuccess()) {
                logger.warn("域名不存在 mailBoxAddress={}", mailBoxAddress);
                return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "邮箱域名不存在");
            }
            result.setDomainName(domain.getData());
            ResponseResult<MailBoxes> mailBox = getMailBoxByName(domain.getData().getDomainNameId(), localPart);
            if (!mailBox.isSuccess()) {
                logger.warn("邮箱不存在 mailBoxAddress={}", mailBoxAddress);
                return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "邮箱不存在");
            }
            result.setMailBox(mailBox.getData());
            ResponseResult<Users> user = usersService.getUserByMailBoxId(mailBox.getData().getMailBoxId());
            if (!user.isSuccess()) {
                logger.warn("用户不存在 mailBoxAddress={}", mailBoxAddress);
                return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "用户不存在");
            }
            result.setUser(user.getData());
            // 同步写入缓存
            cache.setCache(redisKey, result);
            return ResponseResult.success(result);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }
}
