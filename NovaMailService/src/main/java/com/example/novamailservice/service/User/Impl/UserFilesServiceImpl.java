package com.example.novamailservice.service.User.Impl;

import com.example.novamailservice.config.Jwt.JwtProperties;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.UserFileAttachmentAuthorization;
import com.example.novamailservice.dto.Users.UserFileStorageInfo;
import com.example.novamailservice.entity.Users.UserFiles;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.mapper.Mail.EmailMapper;
import com.example.novamailservice.mapper.Users.UserFilesMapper;
import com.example.novamailservice.service.User.UserFilesService;
import com.example.novamailservice.util.MinIo.MinIoBucket;
import com.example.novamailservice.util.MinIo.MinIoFactory;
import jakarta.servlet.http.HttpServletResponse;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.multipart.MultipartFile;

import java.io.InputStream;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.security.DigestInputStream;
import java.security.MessageDigest;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.HexFormat;
import java.util.List;

@Service
public class UserFilesServiceImpl implements UserFilesService {
    private static final Logger logger = LoggerFactory.getLogger(UserFilesServiceImpl.class);
    private static final String storageBucketName = "users";
    private static final String ROOT_PATH = "/";
    private static final int MAX_PATH_LENGTH = 255;
    private static final int MAX_FILE_NAME_LENGTH = 255;
    private static final int MAX_DESCRIPTION_LENGTH = 512;
    private static final long SHARED_LINK_TTL_SECONDS = 30L * 24 * 60 * 60;

    private final UserFilesMapper userFilesMapper;
    private final EmailMapper emailMapper;
    private final MinIoFactory minIoFactory;
    private final JwtProperties jwtProperties;

    public UserFilesServiceImpl(UserFilesMapper userFilesMapper, EmailMapper emailMapper, MinIoFactory minIoFactory, JwtProperties jwtProperties) {
        this.userFilesMapper = userFilesMapper;
        this.emailMapper = emailMapper;
        this.minIoFactory = minIoFactory;
        this.jwtProperties = jwtProperties;
    }

    @Override
    public String getStorageBucketName() {
        return storageBucketName;
    }

    @Override
    public String getAvatarPath(String account) {
        return account + "/avatars";
    }

    @Override
    public ResponseResult<List<UserFiles>> listFiles(Users user) {
        if (user == null || user.getUserId() == null) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        try {
            return ResponseResult.success(userFilesMapper.selectActiveByUserId(user.getUserId()));
        } catch (Exception e) {
            logger.error("用户文件列表获取失败", e);
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "文件列表获取失败");
        }
    }

    @Override
    @Transactional
    public ResponseResult<UserFiles> uploadFile(Users user, MultipartFile file, String filePath, String description) {
        if (user == null || user.getUserId() == null) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (file == null || file.isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "请选择要上传的文件");

        String originalFileName = sanitizeFileName(file.getOriginalFilename());
        if (originalFileName == null || originalFileName.isBlank()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "文件名不能为空");

        String normalizedPath;
        try {
            normalizedPath = normalizeFilePath(filePath);
        } catch (IllegalArgumentException e) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, e.getMessage());
        }

        String safeDescription = limitString(description, MAX_DESCRIPTION_LENGTH);
        String fileHash;
        try {
            fileHash = calculateSha256(file);
        } catch (Exception e) {
            logger.error("文件哈希计算失败", e);
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "文件读取失败");
        }

        Integer maxVersion = userFilesMapper.selectMaxVersion(user.getUserId(), normalizedPath, originalFileName);
        int nextVersion = (maxVersion == null ? 0 : maxVersion) + 1;

        UserFiles userFile = new UserFiles();
        userFile.setUserId(user.getUserId());
        userFile.setFileName(originalFileName);
        userFile.setFileType(resolveContentType(file));
        userFile.setFilePath(normalizedPath);
        userFile.setParentPath(getParentPath(normalizedPath));
        userFile.setStorageBucket(storageBucketName);
        userFile.setFileHash(fileHash);
        userFile.setFileSize(file.getSize());
        userFile.setVersion(nextVersion);
        userFile.setFileStatus("UPLOADING");
        userFile.setDescription(safeDescription);

        try {
            if (userFilesMapper.insert(userFile) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "文件记录创建失败");
            String objectName = buildObjectName(user, userFile);
            MinIoBucket minIoBucket = minIoFactory.openBucket(storageBucketName, true);
            try (InputStream inputStream = file.getInputStream()) {
                minIoBucket.uploadFile(objectName, inputStream, userFile.getFileType(), file.getSize());
            }
            if (userFilesMapper.markActive(user.getUserId(), userFile.getFileId()) <= 0) throw new RuntimeException("文件状态更新失败");
            userFile.setFileStatus("ACTIVE");
            return ResponseResult.success("上传成功", userFilesMapper.selectByIdForUser(user.getUserId(), userFile.getFileId()));
        } catch (Exception e) {
            if (userFile.getFileId() != null) userFilesMapper.updateStatus(user.getUserId(), userFile.getFileId(), "DELETED");
            logger.error("文件上传失败", e);
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "文件上传失败");
        }
    }

    @Override
    public ResponseResult<UserFiles> updateFile(Users user, Long fileId, String fileName, String description) {
        if (user == null || user.getUserId() == null) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (fileId == null || fileId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "文件ID无效");

        UserFiles current = userFilesMapper.selectByIdForUser(user.getUserId(), fileId);
        if (current == null || "DELETED".equals(current.getFileStatus())) return ResponseResult.error(ErrorCode.NOT_FOUND, "文件不存在");

        String safeFileName = sanitizeFileName(fileName);
        if (safeFileName == null || safeFileName.isBlank()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "文件名不能为空");

        Integer maxVersion = userFilesMapper.selectMaxVersion(user.getUserId(), current.getFilePath(), safeFileName);
        int nextVersion = Math.max(current.getVersion() == null ? 1 : current.getVersion() + 1, (maxVersion == null ? 0 : maxVersion) + 1);

        current.setFileName(safeFileName);
        current.setDescription(limitString(description, MAX_DESCRIPTION_LENGTH));
        current.setVersion(nextVersion);

        try {
            if (userFilesMapper.updateMetadata(current) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "文件更新失败");
            return ResponseResult.success("更新成功", userFilesMapper.selectByIdForUser(user.getUserId(), fileId));
        } catch (Exception e) {
            logger.error("文件更新失败", e);
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "文件更新失败");
        }
    }

    @Override
    public ResponseResult<String> deleteFile(Users user, Long fileId) {
        if (user == null || user.getUserId() == null) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (fileId == null || fileId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "文件ID无效");

        UserFiles current = userFilesMapper.selectByIdForUser(user.getUserId(), fileId);
        if (current == null || "DELETED".equals(current.getFileStatus())) return ResponseResult.error(ErrorCode.NOT_FOUND, "文件不存在");

        try {
            if (userFilesMapper.updateStatus(user.getUserId(), fileId, "DELETED") <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "文件删除失败");
            return ResponseResult.success("删除成功");
        } catch (Exception e) {
            logger.error("文件删除失败", e);
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "文件删除失败");
        }
    }

    @Override
    public ResponseResult<List<UserFileAttachmentAuthorization>> listAttachmentAuthorizations(Users user, Long fileId) {
        if (user == null || user.getUserId() == null) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (fileId == null || fileId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "文件ID无效");

        UserFiles current = userFilesMapper.selectByIdForUser(user.getUserId(), fileId);
        if (current == null || "DELETED".equals(current.getFileStatus())) return ResponseResult.error(ErrorCode.NOT_FOUND, "文件不存在");

        try {
            return ResponseResult.success(emailMapper.listUserFileAttachmentAuthorizations(user.getUserId(), fileId));
        } catch (Exception e) {
            logger.error("用户文件附件授权列表获取失败", e);
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "附件授权列表获取失败");
        }
    }

    @Override
    public ResponseResult<String> revokeAttachmentAuthorization(Users user, Long fileId, Long attachmentId) {
        if (user == null || user.getUserId() == null) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (fileId == null || fileId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "文件ID无效");
        if (attachmentId == null || attachmentId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "附件ID无效");

        try {
            int affected = emailMapper.deleteUserFileAttachmentAuthorization(user.getUserId(), fileId, attachmentId);
            if (affected <= 0) return ResponseResult.error(ErrorCode.NOT_FOUND, "附件授权不存在或无权撤销");
            return ResponseResult.success("授权已撤销");
        } catch (Exception e) {
            logger.error("用户文件附件授权撤销失败", e);
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "附件授权撤销失败");
        }
    }

    @Override
    public String createSharedDownloadUrl(Users user, Long fileId, String baseUrl) {
        if (user == null || user.getUserId() == null) return null;
        if (fileId == null || fileId <= 0) return null;

        UserFiles userFile = userFilesMapper.selectByIdForUser(user.getUserId(), fileId);
        if (userFile == null || !"ACTIVE".equals(userFile.getFileStatus())) return null;

        long expiresAt = Instant.now().plus(SHARED_LINK_TTL_SECONDS, ChronoUnit.SECONDS).getEpochSecond();
        String token = signSharedLink(fileId, expiresAt);
        if (token == null || token.isBlank()) return null;

        String normalizedBaseUrl = baseUrl == null ? "" : baseUrl.trim();
        if (normalizedBaseUrl.endsWith("/")) normalizedBaseUrl = normalizedBaseUrl.substring(0, normalizedBaseUrl.length() - 1);
        String path = "/userFiles/shared/download?fileId=" + fileId + "&expires=" + expiresAt + "&token=" + token;
        return normalizedBaseUrl.isBlank() ? path : normalizedBaseUrl + path;
    }

    @Override
    public void downloadFile(Users user, Long fileId, HttpServletResponse response) throws Exception {
        if (user == null || user.getUserId() == null) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        if (fileId == null || fileId <= 0) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }

        UserFiles userFile = userFilesMapper.selectByIdForUser(user.getUserId(), fileId);
        if (userFile == null || !"ACTIVE".equals(userFile.getFileStatus())) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }

        UserFileStorageInfo storageInfo = toStorageInfo(user, userFile);
        streamFile(storageInfo, response, false);
    }

    @Override
    public void downloadSharedFile(Long fileId, String token, HttpServletResponse response) throws Exception {
        if (fileId == null || fileId <= 0 || token == null || token.isBlank()) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }

        SharedLinkToken parsedToken = parseSharedLinkToken(token);
        long now = Instant.now().getEpochSecond();
        if (parsedToken == null || parsedToken.expiresAt() < now || !isSharedLinkTokenValid(fileId, parsedToken)) {
            response.setStatus(HttpServletResponse.SC_FORBIDDEN);
            return;
        }

        streamReferencedFile(fileId, response, false);
    }

    @Override
    public void previewFile(Users user, Long fileId, HttpServletResponse response) throws Exception {
        if (user == null || user.getUserId() == null) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        if (fileId == null || fileId <= 0) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }

        UserFiles userFile = userFilesMapper.selectByIdForUser(user.getUserId(), fileId);
        if (userFile == null || !"ACTIVE".equals(userFile.getFileStatus())) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }

        UserFileStorageInfo storageInfo = toStorageInfo(user, userFile);
        streamFile(storageInfo, response, true);
    }

    @Override
    public void streamReferencedFile(Long fileId, HttpServletResponse response, boolean inline) throws Exception {
        if (fileId == null || fileId <= 0) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }

        UserFileStorageInfo storageInfo = userFilesMapper.selectActiveStorageInfoById(fileId);
        if (storageInfo == null) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }

        streamFile(storageInfo, response, inline);
    }

    private UserFileStorageInfo toStorageInfo(Users user, UserFiles userFile) {
        UserFileStorageInfo storageInfo = new UserFileStorageInfo();
        storageInfo.setFileId(userFile.getFileId());
        storageInfo.setUserId(userFile.getUserId());
        storageInfo.setAccount(user.getAccount());
        storageInfo.setFileName(userFile.getFileName());
        storageInfo.setFileType(userFile.getFileType());
        storageInfo.setStorageBucket(userFile.getStorageBucket());
        storageInfo.setFileSize(userFile.getFileSize());
        storageInfo.setFileStatus(userFile.getFileStatus());
        return storageInfo;
    }

    private void streamFile(UserFileStorageInfo storageInfo, HttpServletResponse response, boolean inline) throws Exception {
        MinIoBucket minIoBucket;
        try {
            minIoBucket = minIoFactory.openBucket(storageInfo.getStorageBucket());
        } catch (Exception e) {
            response.setStatus(HttpServletResponse.SC_SERVICE_UNAVAILABLE);
            return;
        }

        String objectName = buildObjectName(storageInfo.getAccount(), storageInfo.getFileId());
        if (!minIoBucket.fileExists(objectName)) {
            response.setStatus(HttpServletResponse.SC_NOT_FOUND);
            return;
        }

        String contentType = storageInfo.getFileType();
        if (contentType == null || contentType.isBlank()) contentType = minIoBucket.getFileType(objectName);
        if (contentType == null || contentType.isBlank()) contentType = "application/octet-stream";

        response.setContentType(contentType);
        if (storageInfo.getFileSize() != null) response.setHeader("Content-Length", String.valueOf(storageInfo.getFileSize()));
        response.setHeader("Content-Disposition", buildContentDisposition(storageInfo.getFileName(), inline));
        response.setHeader("X-Content-Type-Options", "nosniff");

        try (InputStream inputStream = minIoBucket.downloadFile(objectName)) {
            inputStream.transferTo(response.getOutputStream());
        }
    }

    private String buildObjectName(Users user, UserFiles userFile) {
        return buildObjectName(user.getAccount(), userFile.getFileId());
    }

    private String buildObjectName(String account, Long fileId) {
        return account + "/filebox/" + fileId + "/content";
    }

    private String buildContentDisposition(String fileName, boolean inline) {
        String safeFileName = fileName == null || fileName.isBlank() ? "attachment" : fileName;
        String fallbackName = safeFileName.replace("\\", "_").replace("\"", "'");
        fallbackName = fallbackName.chars()
                .mapToObj(ch -> ch >= 0x20 && ch <= 0x7E ? String.valueOf((char) ch) : "_")
                .reduce("", String::concat);
        if (fallbackName.isBlank()) fallbackName = "download";
        String encodedName = URLEncoder.encode(safeFileName, StandardCharsets.UTF_8).replace("+", "%20");
        return (inline ? "inline" : "attachment") + "; filename=\"" + fallbackName + "\"; filename*=UTF-8''" + encodedName;
    }

    private String signSharedLink(Long fileId, long expiresAt) {
        String secret = jwtProperties.getSecret();
        if (secret == null || secret.isBlank()) return null;
        String signature = sha256Hex(fileId + ":" + expiresAt + ":" + secret);
        return expiresAt + "." + signature;
    }

    private SharedLinkToken parseSharedLinkToken(String token) {
        String value = token == null ? "" : token.trim();
        int separatorIndex = value.indexOf('.');
        if (separatorIndex <= 0 || separatorIndex >= value.length() - 1) return null;
        try {
            long expiresAt = Long.parseLong(value.substring(0, separatorIndex));
            String signature = value.substring(separatorIndex + 1);
            if (!signature.matches("[0-9a-fA-F]{64}")) return null;
            return new SharedLinkToken(expiresAt, signature.toLowerCase());
        } catch (Exception e) {
            return null;
        }
    }

    private boolean isSharedLinkTokenValid(Long fileId, SharedLinkToken token) {
        String expected = signSharedLink(fileId, token.expiresAt());
        if (expected == null) return false;
        return MessageDigest.isEqual(expected.getBytes(StandardCharsets.UTF_8), (token.expiresAt() + "." + token.signature()).getBytes(StandardCharsets.UTF_8));
    }

    private String sha256Hex(String value) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            return HexFormat.of().formatHex(digest.digest(value.getBytes(StandardCharsets.UTF_8)));
        } catch (Exception e) {
            throw new IllegalStateException("SHA-256 unavailable", e);
        }
    }

    private record SharedLinkToken(long expiresAt, String signature) {
    }

    private String calculateSha256(MultipartFile file) throws Exception {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        try (InputStream inputStream = file.getInputStream(); DigestInputStream digestInputStream = new DigestInputStream(inputStream, digest)) {
            digestInputStream.transferTo(OutputStreamStub.INSTANCE);
        }
        return HexFormat.of().formatHex(digest.digest());
    }

    private String resolveContentType(MultipartFile file) {
        String contentType = file.getContentType();
        if (contentType == null || contentType.isBlank()) return "application/octet-stream";
        if (contentType.length() > 50) return "application/octet-stream";
        return contentType;
    }

    private String normalizeFilePath(String path) {
        String normalized = path == null ? ROOT_PATH : path.trim().replace("\\", "/").replaceAll("/+", "/");
        if (normalized.isBlank() || ROOT_PATH.equals(normalized)) return ROOT_PATH;
        if (!normalized.startsWith(ROOT_PATH)) normalized = ROOT_PATH + normalized;
        if (normalized.endsWith(ROOT_PATH) && normalized.length() > 1) normalized = normalized.substring(0, normalized.length() - 1);
        if (normalized.length() > MAX_PATH_LENGTH) throw new IllegalArgumentException("文件路径过长");
        return normalized;
    }

    private String getParentPath(String path) {
        String normalized = normalizeFilePath(path);
        if (ROOT_PATH.equals(normalized)) return null;
        int lastSlash = normalized.lastIndexOf("/");
        if (lastSlash <= 0) return ROOT_PATH;
        return normalized.substring(0, lastSlash);
    }

    private String sanitizeFileName(String fileName) {
        if (fileName == null) return null;
        String normalized = fileName.replace("\\", "/");
        int slashIndex = normalized.lastIndexOf("/");
        if (slashIndex >= 0) normalized = normalized.substring(slashIndex + 1);
        normalized = normalized.trim();
        if (normalized.length() > MAX_FILE_NAME_LENGTH) normalized = normalized.substring(0, MAX_FILE_NAME_LENGTH);
        return normalized;
    }

    private String limitString(String value, int maxLength) {
        if (value == null) return "";
        String trimmed = value.trim();
        if (trimmed.codePointCount(0, trimmed.length()) <= maxLength) return trimmed;
        int endIndex = trimmed.offsetByCodePoints(0, maxLength);
        return trimmed.substring(0, endIndex);
    }

    private static class OutputStreamStub extends java.io.OutputStream {
        private static final OutputStreamStub INSTANCE = new OutputStreamStub();

        @Override
        public void write(int b) {
        }

        @Override
        public void write(byte[] b, int off, int len) {
        }
    }
}
