package com.example.novamailservice.service.User.Impl;

import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Result.ServiceException;
import com.example.novamailservice.dto.Users.HistoryAvatar;
import com.example.novamailservice.entity.Users.UserProfile;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.mapper.Users.UserProfileMapper;
import com.example.novamailservice.service.User.UserFilesService;
import com.example.novamailservice.service.User.UserProfileService;
import com.example.novamailservice.util.AvatarProcessing;
import com.example.novamailservice.util.Cache;
import com.example.novamailservice.util.MinIo.FileInfo;
import com.example.novamailservice.util.MinIo.MinIoBucket;
import com.example.novamailservice.util.MinIo.MinIoFactory;
import lombok.AllArgsConstructor;
import lombok.Data;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Isolation;
import org.springframework.transaction.annotation.Propagation;
import org.springframework.transaction.annotation.Transactional;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.HashSet;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

@Service
public class UserProfileServiceImpl implements UserProfileService {
    private final Logger logger = LoggerFactory.getLogger(UserProfileServiceImpl.class.getName());
    private final UserFilesService userFilesService;
    private final UserProfileMapper userProfileMapper;
    private final Cache cache;
    private final MinIoFactory minIoFactory;
    private static final Pattern BASE64_PATTERN = Pattern.compile("data:(image/\\w+);base64,");
    private static final String UserProfileCachePrefix = RedisConfig.NovaCache + "UserProfileCache:";
    private static final String redisKeyPrefixAccount = UserProfileCachePrefix + "Account:";

    @Data
    @AllArgsConstructor
    public static class AvatarNameResult {
        private String objectName;
        private int index;
    }

    @Autowired
    public UserProfileServiceImpl(UserFilesService userFilesService, UserProfileMapper userProfileMapper, Cache cache, MinIoFactory minIoFactory) {
        this.userFilesService = userFilesService;
        this.userProfileMapper = userProfileMapper;
        this.cache = cache;
        this.minIoFactory = minIoFactory;
    }

    @Override
    public boolean createUserProfile(UserProfile userProfile) {
        return userProfileMapper.createUserProfile(userProfile) > 0;
    }

    @Override
    public ResponseResult<UserProfile> getUserProfileByAccount(String account) {
        String redisKey = redisKeyPrefixAccount + account;
        ResponseResult<UserProfile> cachedResult = cache.getCache(redisKey, UserProfile.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 查询数据库
        try {
            UserProfile userProfile = userProfileMapper.getUserProfile(account);
            if (userProfile == null) return ResponseResult.error(ErrorCode.USER_NOT_FOUND, "用户信息不存在");

            // 同步写入缓存
            cache.setCache(redisKey, userProfile);
            return ResponseResult.success(userProfile);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public String getAvatarObjectName(String account) {
        ResponseResult<UserProfile> userProfileResult = getUserProfileByAccount(account);
        if (!userProfileResult.isSuccess()) return null; // 获取用户信息失败
        UserProfile userProfile = userProfileResult.getData();
        if (userProfile == null) return null; // 用户信息为空
        if (userProfile.getAvatar() == null || userProfile.getAvatar() <= 0) return null; // 用户头像未设置
        return userFilesService.getAvatarPath(account)+ "/" + userProfile.getAvatar() + "_avatar.png";
    }

    // 截取字符
    private String limitString(String string, int maxLength) {
        if (string == null) return null;

        // 去除前后空格
        string = string.trim();

        // 按 Unicode 字符数判断（避免中文/emoji截断问题）
        int length = string.codePointCount(0, string.length());
        if (length <= maxLength) return string;

        // 找到截断位置（安全）
        int endIndex = string.offsetByCodePoints(0, maxLength);

        return string.substring(0, endIndex);
    }

    // 从文件名中解析出 头像索引
    private int parseAvatarIndex(String fileName) {
        int pos = fileName.indexOf("_avatar");
        if (pos <= 0) return 0;

        try {
            return Integer.parseInt(fileName.substring(0, pos));
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    // 获取下一个头像名称
    private AvatarNameResult getNextAvatarObjectName(String account) {
        int maxIndex = 6;
        String avatarPath = userFilesService.getAvatarPath(account);
        String bucketName = userFilesService.getStorageBucketName();

        try {
            MinIoBucket bucket = minIoFactory.openBucket(bucketName);
            List<FileInfo> infos = bucket.listFiles(avatarPath, MinIoBucket.SortOrder.DESC);

            // 收集 index
            Set<Integer> used = new HashSet<>();
            if (infos != null) {
                for (FileInfo info : infos) {
                    String fileName = bucket.getFileName(info.getObjectName());
                    int idx = parseAvatarIndex(fileName);
                    if (idx > 0 && idx <= maxIndex) used.add(idx);
                }
            }

            // 查找 最小可用 index
            for (int i = 1; i <= maxIndex; i++) if (!used.contains(i)) return new AvatarNameResult(avatarPath + "/" + i + "_avatar.png", i);

            // 超出限制 返回 最旧的 (上传直接覆盖)
            if (!infos.isEmpty()) {
                FileInfo oldest = infos.get(infos.size() - 1); // DESC → 最后是最旧
                String fileName = bucket.getFileName(oldest.getObjectName());
                int idx = parseAvatarIndex(fileName);
                if (idx > 0 && idx <= maxIndex) return new AvatarNameResult(oldest.getObjectName(), idx);
            }
        } catch (Exception e) {
            logger.error("获取头像名称失败", e);
        }

        return new AvatarNameResult(avatarPath + "/1_avatar.png", 1);
    }

    @Override
    public ResponseResult<List<HistoryAvatar>> getUserHistoryAvatars(String account) {
        String avatarPath  = userFilesService.getAvatarPath(account);
        String bucketName = userFilesService.getStorageBucketName();

        // 尝试打开 MinIO 桶
        MinIoBucket minIoBucket;
        try {
            minIoBucket = minIoFactory.openBucket(bucketName);
        } catch (Exception e) {
            return ResponseResult.error(ErrorCode.NOT_FOUND, "存储桶不存在");
        }

        // 当前头像 index
        int currentIndex = 0;
        ResponseResult<UserProfile> userResult = getUserProfileByAccount(account);
        if (userResult.isSuccess()) currentIndex = userResult.getData().getAvatar();

        try {
            // 获取文件列表
            List<FileInfo> avatarInfos = minIoBucket.listFiles(avatarPath, MinIoBucket.SortOrder.DESC);
            List<HistoryAvatar> historyAvatars = new ArrayList<>();

            for (FileInfo info : avatarInfos) {
                // 解析类型
                String type;
                try {
                    type = minIoBucket.getFileType(info.getObjectName());
                } catch (Exception e) { type = null; }
                if (type == null || type.equalsIgnoreCase("application/octet-stream")) type = "image/png";

                // 解析 index
                int avatarIdx = parseAvatarIndex(minIoBucket.getFileName(info.getObjectName()));
                if (avatarIdx > 0) {
                    // 下载并转换 Base64
                    try (InputStream avatarStream = minIoBucket.downloadFile(info.getObjectName())) {
                        byte[] data = avatarStream.readAllBytes();

                        String base64 = Base64.getEncoder().encodeToString(data);

                        historyAvatars.add(new HistoryAvatar(
                                minIoBucket.getFileName(info.getObjectName()),
                                "data:" + type + ";base64," + base64,
                                currentIndex != 0 && avatarIdx == currentIndex,
                                info.getLastModified()
                        ));
                    } catch (Exception e) {
                        logger.error("头像读取失败：{}", info.getObjectName(), e);
                    }
                }
            }
            return ResponseResult.success(historyAvatars);
        } catch (Exception e) {
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "文件获取失败");
        }
    }

    @Override
    public ResponseResult<String> updateNickname(Users user, String nickname) {
        String redisKey = redisKeyPrefixAccount + user.getAccount();

        // 字数限制
        nickname = limitString(nickname, 16);
        if (nickname == null || nickname.isEmpty()) return ResponseResult.error("昵称不能为空");

        // 用户名称校验
        ResponseResult<String> validateNickname = UserProfile.validateNickname(nickname);
        if (!validateNickname.isSuccess()) return ResponseResult.error(ErrorCode.USER_INVALID, validateNickname.getMessage());

        // 更新用户数据
        try {
            cache.removeCache(redisKey);
            if (userProfileMapper.updateNickname(user.getUserId(), nickname) <= 0) return ResponseResult.error("修改昵称失败");
            cache.removeCache(redisKey);

            return ResponseResult.success("修改成功");
        } catch (Exception e) {
            return ResponseResult.error("修改昵称失败");
        }
    }

    @Override
    @Transactional(propagation = Propagation.REQUIRED, isolation = Isolation.DEFAULT)
    public ResponseResult<String> updateAvatar(Users user, String base64Data, String fileName) {
        String redisKey = redisKeyPrefixAccount + user.getAccount();
        String bucketName = userFilesService.getStorageBucketName();

        // 获取账号信息
        ResponseResult<UserProfile> userProfile = getUserProfileByAccount(user.getAccount());
        if (!userProfile.isSuccess()) return ResponseResult.error(ErrorCode.NOT_FOUND, "信息不存在");

        int currentIndex = userProfile.getData().getAvatar();
        try {
            // 尝试打开 MinIO 桶
            MinIoBucket minIoBucket = minIoFactory.openBucket(bucketName, true);

            if (base64Data != null && !base64Data.isBlank()) {
                AvatarNameResult nextAvatarObjectName = getNextAvatarObjectName(user.getAccount());
                Matcher matcher = BASE64_PATTERN.matcher(base64Data);
                if (!matcher.find()) return ResponseResult.error("头像格式错误");

                // 压缩头像
                base64Data = AvatarProcessing.AvatarProcessing(base64Data, 60, 60);
                if (base64Data == null) return ResponseResult.error("头像处理失败");

                // 解码Base64数据
                byte[] fileBytes;
                try {
                    int comma = base64Data.indexOf(",");
                    fileBytes = Base64.getDecoder().decode(base64Data.substring(comma + 1));
                } catch (Exception e) {
                    return ResponseResult.error("Base64解析失败");
                }

                // 更新数据与上传文件
                try (InputStream fileInputStream = new ByteArrayInputStream(fileBytes)) {
                    // 更新用户数据
                    cache.removeCache(redisKey);
                    if (userProfileMapper.updateUserAvatar(user.getUserId(), nextAvatarObjectName.getIndex()) <= 0) throw new RuntimeException("修改用户头像失败");
                    cache.removeCache(redisKey);

                    // 上传文件
                    minIoBucket.uploadFile(nextAvatarObjectName.getObjectName(), fileInputStream, "image/png", fileBytes.length);
                } catch (Exception e) { throw new RuntimeException("头像上传失败"); }

                return ResponseResult.success("头像上传成功");
            }

            if (fileName == null || fileName.isBlank()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
            String objectName = userFilesService.getAvatarPath(user.getAccount()) + "/" + fileName;
            if (!minIoBucket.fileExists(objectName)) return ResponseResult.error("历史头像不存在");

            int newIndex = parseAvatarIndex(fileName);
            if (newIndex == currentIndex) return ResponseResult.error("当前头像一致");

            // 更新用户数据
            try {
                cache.removeCache(redisKey);
                if (userProfileMapper.updateUserAvatar(user.getUserId(), newIndex) <= 0) throw new RuntimeException("修改用户头像失败");
                cache.removeCache(redisKey);

                // 更新头像使用时间
                minIoBucket.updateFileTime(objectName);
            } catch (Exception e) {
                throw new RuntimeException("修改用户头像失败");
            }

            return ResponseResult.success("修改成功");
        } catch (Exception e) {
            logger.error("修改用户头像失败", e);
            throw new RuntimeException("修改用户头像失败");
        }
    }

    @Override
    public ResponseResult<String> updateResume(Users user, String resume) {
        String redisKey = redisKeyPrefixAccount + user.getAccount();

        // 字数限制
        resume = limitString(resume, 200);
        if (resume == null || resume.isEmpty()) return ResponseResult.error(ErrorCode.USER_INVALID,"简介不能为空");

        // 更新用户数据
        try {
            cache.removeCache(redisKey);
            if (userProfileMapper.updateResume(user.getUserId(), resume) <= 0) return ResponseResult.error("简介昵称失败");
            cache.removeCache(redisKey);

            return ResponseResult.success("修改成功");
        } catch (Exception e) {
            return ResponseResult.error("简介昵称失败");
        }
    }

    @Override
    public ResponseResult<String> updateDefaultMailBox(Users user, Long mailBoxId) {
        if (user == null || user.getUserId() == null || mailBoxId == null || mailBoxId <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        String redisKey = redisKeyPrefixAccount + user.getAccount();
        try {
            cache.removeCache(redisKey);
            if (userProfileMapper.updateDefaultMailBox(user.getUserId(), mailBoxId) <= 0) {
                return ResponseResult.error(ErrorCode.DATABASE_ERROR, "默认邮箱更新失败");
            }
            cache.removeCache(redisKey);
            return ResponseResult.success("设置成功");
        } catch (Exception e) {
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "默认邮箱更新失败");
        }
    }
}
