package com.example.novamailservice.service.Authentication;

import com.example.novamailservice.config.Filter.RequestContext;
import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.DeviceInfo;
import com.example.novamailservice.dto.IpInfo;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Result.ServiceException;
import com.example.novamailservice.entity.Mail.MailBoxes;
import com.example.novamailservice.entity.Users.UserLoginLog;
import com.example.novamailservice.entity.Users.UserProfile;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.Mail.MailBoxesService;
import com.example.novamailservice.service.User.UserLoginLogService;
import com.example.novamailservice.service.User.UserProfileService;
import com.example.novamailservice.service.User.UserSignatureService;
import com.example.novamailservice.service.User.UsersService;
import com.example.novamailservice.util.Cache;
import com.example.novamailservice.util.JwtTokenProvider;
import org.jetbrains.annotations.NotNull;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Duration;
import java.time.LocalDateTime;
import java.time.ZoneId;

@Service
public class AuthenticationServiceImpl implements AuthenticationService {
    private final JwtTokenProvider jwtTokenProvider;
    private final UsersService usersService;
    private final UserProfileService userProfileService;
    private final UserSignatureService userSignatureService;
    private final MailBoxesService mailBoxesService;
    private final UserLoginLogService userLoginLogService;
    private final Cache cache;

    // 最大密码错误次数
    private static final int maxPasswordErrorCount = 6;
    // 重复锁定增加时长 5 分钟 * 锁定次数
    private static final Duration userLockedDurationIncrease = Duration.ofMinutes(5);

    private static final String AuthenticationCachePrefix = RedisConfig.NovaCache + "AuthenticationCache:";
    private static final String redisKeyPrefixUserLocked = AuthenticationCachePrefix + "UserLocked:";

    @Autowired
    public AuthenticationServiceImpl(JwtTokenProvider jwtTokenProvider, UsersService usersService, UserProfileService userProfileService, UserSignatureService userSignatureService, MailBoxesService mailBoxesService, UserLoginLogService userLoginLogService, Cache cache) {
        this.jwtTokenProvider = jwtTokenProvider;
        this.usersService = usersService;
        this.userProfileService = userProfileService;
        this.userSignatureService = userSignatureService;
        this.mailBoxesService = mailBoxesService;
        this.userLoginLogService = userLoginLogService;
        this.cache = cache;
    }

    @Transactional
    @Override // 注册
    public ResponseResult<String> register(String account, String password) {
        // 账号校验
        ResponseResult<String> validateAccount = Users.validateAccount(account);
        if (!validateAccount.isSuccess())
            throw new ServiceException(ErrorCode.USER_INVALID, validateAccount.getMessage());
        // 密码校验
        ResponseResult<String> validatePassword = Users.validatePassword(password);
        if (!validatePassword.isSuccess())
            throw new ServiceException(ErrorCode.USER_INVALID, validatePassword.getMessage());

        // 账号查重
        ResponseResult<Users> user = usersService.getUserByAccount(account);
        if (user.isSuccess()) throw new ServiceException(ErrorCode.USER_EXISTS, "用户已存在");
        if (user.getCode() == ErrorCode.DATABASE_ERROR || user.getCode() == ErrorCode.CACHE_ERROR)
            throw new ServiceException(user.getCode(), user.getMessage());

        // 创建用户
        Users newUser = new Users();
        newUser.setAccount(account);
        newUser.setPassword(password);
        newUser.setIsAdmin(false);
        if (!usersService.createUser(newUser))
            throw new ServiceException(ErrorCode.DATABASE_ERROR, "无法插入用户信息");

        // 创建邮箱
        ResponseResult<MailBoxes> newMailBox = mailBoxesService.createMailBox(newUser, null, null, 100000);

        // 创建用户信息
        UserProfile newProfile = createUserProfile(account, newUser, newMailBox);

        // 创建用户资料
        if (!userProfileService.createUserProfile(newProfile))
            throw new ServiceException(ErrorCode.DATABASE_ERROR, "无法插入用户资料");

        // 添加默认签名
        if (!userSignatureService.createDefaultSignature(newUser))
            throw new ServiceException(ErrorCode.DATABASE_ERROR, "无法插入默认签名");

        if (!newMailBox.isSuccess()) return ResponseResult.error(ErrorCode.MAILBOX_NOT_FOUND, "注册成功，但是邮箱创建失败，需要用户手动创建");
        return ResponseResult.success("注册成功");
    }

    @NotNull
    private static UserProfile createUserProfile(String account, Users newUser, ResponseResult<MailBoxes> newMailBox) {
        UserProfile newProfile = new UserProfile();
        newProfile.setUserId(newUser.getUserId());
        newProfile.setNickname(account);

        // 获取IP归属地
        IpInfo ipInfo = RequestContext.getCurrentIp();
        if (ipInfo != null) {
            String region = ipInfo.getIpRegion();
            if (region != null && !region.isBlank()) {
                String[] parts = region.split("\\|");
                StringBuilder address = new StringBuilder();
                for (int i = 0; i <= 3 && i < parts.length; i++) { // 只拼国家、省、市（索引0~3）
                    String part = parts[i];
                    if (part != null && !part.isBlank() && !"0".equals(part)) {
                        if (!address.isEmpty()) address.append(" ");
                        address.append(part);
                    }
                }
                newProfile.setAddress(!address.isEmpty() ? address.toString() : "未知");
            } else {
                newProfile.setAddress("未知"); // IP为空或无法解析
            }
        }

        // 设置默认邮箱
        if (newMailBox.isSuccess()) newProfile.setDefaultMailBoxId(newMailBox.getData().getMailBoxId());
        return newProfile;
    }

    private void InitializeLoginLog(String account) {
        UserLoginLog log = new UserLoginLog();

        // 账号
        log.setAccount(account);

        // 时间
        log.setLoginTime(LocalDateTime.now());

        // IP 信息（来自 Filter）
        IpInfo ipInfo = RequestContext.getCurrentIp();
        if (ipInfo != null) {
            log.setClientIp(ipInfo.getIpAddress());
            log.setIpLocation(ipInfo.getIpRegion());
        }

        // 登录设备
        DeviceInfo device = RequestContext.getDevice();
        if (device != null) {
            // 网络
            log.setNetworkType(device.getNetworkType());
            log.setCarrier(device.getCarrier());

            // 设备消息
            log.setUserAgent(device.getRawUserAgent());
            log.setDeviceType(device.getDeviceType() != null  ? device.getDeviceType().getDisplayName() : "UNKNOWN");


            // 系统
            log.setOsFamily(device.getOsFamily());
            log.setOsVersion(device.getOsVersion());
            log.setOsMajor(device.getOsMajor());
            log.setOsMinor(device.getOsMinor());

            // 浏览器
            log.setBrowser(device.getBrowser());
            log.setBrowserVersion(device.getBrowserVersion());
            log.setBrowserMajor(device.getBrowserMajor());

            // 设备
            log.setDeviceBrand(device.getDeviceBrand());
            log.setDeviceModel(device.getDeviceModel());
            log.setDeviceCode(device.getDeviceCode());

            // 风控
            log.setIsBot(device.isBot());
            log.setPlatform(device.getPlatform());
        }

        // 暂存 日志
        RequestContext.setLoginLog(log);
    }

    private void UploadLoginLog(String failReason, Long userId) {
        UserLoginLog log = RequestContext.getLoginLog();
        if (log == null) return;

        if (userId != null) usersService.updateLastLoginTime(userId, log.getAccount());

        log.setLoginResult((userId == null || userId <= 0) ? "FAIL" : "SUCCESS");
        log.setFailReason(failReason);
        log.setResponseTime((int)(System.currentTimeMillis() - log.getLoginTime().atZone(ZoneId.systemDefault()).toInstant().toEpochMilli()));

        userLoginLogService.insert(log);
    }

    private void UploadLoginLog(String failReason) {
        UploadLoginLog(failReason, null);
    }

    @Override
    public ResponseResult<String> login(String account, String password) {
        if (account == null || account.isEmpty()) return ResponseResult.error(ErrorCode.USER_INVALID, "账号不能为空");
        if (password == null || password.isEmpty()) return ResponseResult.error(ErrorCode.USER_INVALID, "密码不能为空");

        InitializeLoginLog(account); // 初始化登录日志

        String redisKey = redisKeyPrefixUserLocked + account + ":";
        String redisKeyLocked = redisKey + "Locked";
        String redisKeyErrorCount = redisKey + "ErrorCount";

        // 查询账号是否锁定
        ResponseResult<Long> isLocked = cache.getCache(redisKeyLocked, Long.class, false);
        if (isLocked.isSuccess()) {
            Long lockedTime = isLocked.getData();
            if (lockedTime != null && lockedTime > 0) {
                String[] units = {"秒", "分钟", "小时", "天"};
                long[] scales = {60, 60, 24};
                int i = 0;
                double time = lockedTime;
                for (; i < scales.length && time >= scales[i]; i++) time /= scales[i];
                if (i >= units.length) i = units.length - 1; // 防止越界

                UploadLoginLog("账号已锁定，" + Math.round(time) + units[i] + " 后自动解锁");
                return ResponseResult.error(ErrorCode.USER_LOCKED, "账号已锁定，可以到安全中心手动解锁，或等待 " + Math.round(time) + units[i] + " 后自动解锁");
            } else {
                // 移除锁定缓存
                cache.removeCache(redisKeyLocked);
            }
        } else if (isLocked.getCode() == ErrorCode.CACHE_ERROR) {
            UploadLoginLog("缓存查询错误");
            return ResponseResult.error(ErrorCode.CACHE_ERROR, isLocked.getMessage());
        }

        // 查询账号
        ResponseResult<Users> userResult = usersService.getUserByAccount(account);
        if (!userResult.isSuccess()) {
            int code = userResult.getCode();
            String message = userResult.getMessage();
            if (code == ErrorCode.USER_NOT_FOUND || code == ErrorCode.DATABASE_ERROR) return ResponseResult.error(ErrorCode.USER_PASSWORD_ERROR, "账号或密码错误");

            UploadLoginLog("账号不存在");

            return ResponseResult.error(code, message);
        }

        Users user = userResult.getData();
        // 判断账号状态
        switch (user.getStatus()) {
            case "NORMAL" -> { // 正常
                // 密码校验
                if (!jwtTokenProvider.matchesPassword(password, user.getPassword())) {
                    // 获取错误次数
                    ResponseResult<Long> errorCount = cache.getCache(redisKeyErrorCount, Long.class, false);
                    if (errorCount.isSuccess()) {
                        if ((errorCount.getData() + 1) >= maxPasswordErrorCount) {
                            // 锁定倍数
                            long multiple = errorCount.getData() + 2 - maxPasswordErrorCount;
                            Long lockedSeconds = multiple * userLockedDurationIncrease.getSeconds();
                            // 锁定, 增加有效期防止提前过期
                            cache.setCache(redisKeyLocked, lockedSeconds, Duration.ofSeconds(lockedSeconds + lockedSeconds));
                        }
                    }
                    // 缓存错误次数
                    cache.setCache(redisKeyErrorCount, errorCount.isSuccess() ? errorCount.getData() + 1 : 1, Duration.ofDays(1));

                    UploadLoginLog("密码错误");
                    return ResponseResult.error(ErrorCode.USER_PASSWORD_ERROR, "账号或密码错误");
                }
                // 密码正确, 清理缓存
                cache.removeCache(redisKeyErrorCount);
                cache.removeCache(redisKeyLocked);

                try {
                    // 生成 Token
                    String token = jwtTokenProvider.generateToken(account);
                    // 写入登录日志
                    UploadLoginLog("登录成功", user.getUserId());
                    // 返回 Token
                    return ResponseResult.success(token);
                } catch (IllegalArgumentException e) {
                    UploadLoginLog(e.getMessage());
                    return ResponseResult.error(ErrorCode.USER_INVALID, e.getMessage());
                } catch (IllegalStateException e) {
                    UploadLoginLog("JWT配置异常");
                    return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "JWT配置异常");
                } catch (Exception e) {
                    UploadLoginLog("Token生成失败");
                    return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "登录失败");
                }
            }
            case "DISABLED" -> {
                // 账号被封禁
                UploadLoginLog("账号被禁用");
                return ResponseResult.error(ErrorCode.USER_DISABLED, "账号被禁用");
            }
            case "DELETED" -> {
                // 判断 删除时间 小于 7 天可以恢复登录

                // 账号删除
                UploadLoginLog("账号已被删除");
                return ResponseResult.error(ErrorCode.USER_PASSWORD_ERROR, "账号或密码错误");
            }
            default -> {
                UploadLoginLog("账号状态异常");
                return ResponseResult.error(ErrorCode.USER_INVALID, "账号状态异常");
            }
        }
    }

    @Override
    public ResponseResult<String> logout() {
        try {
            // 获取当前登录账号
            String account = RequestContext.getCurrentAccount();
            jwtTokenProvider.removeToken(account);
            return ResponseResult.success();
        } catch (Exception e) {
            return ResponseResult.error("退出失败");
        }
    }
}
