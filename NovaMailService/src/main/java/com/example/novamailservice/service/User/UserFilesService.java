package com.example.novamailservice.service.User;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.UserFileAttachmentAuthorization;
import com.example.novamailservice.entity.Users.UserFiles;
import com.example.novamailservice.entity.Users.Users;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.web.multipart.MultipartFile;

import java.util.List;

public interface UserFilesService {
    String getStorageBucketName();
    String getAvatarPath(String account);
    ResponseResult<List<UserFiles>> listFiles(Users user);
    ResponseResult<UserFiles> uploadFile(Users user, MultipartFile file, String filePath, String description);
    ResponseResult<UserFiles> updateFile(Users user, Long fileId, String fileName, String description);
    ResponseResult<String> deleteFile(Users user, Long fileId);
    ResponseResult<List<UserFileAttachmentAuthorization>> listAttachmentAuthorizations(Users user, Long fileId);
    ResponseResult<String> revokeAttachmentAuthorization(Users user, Long fileId, Long attachmentId);
    String createSharedDownloadUrl(Users user, Long fileId, String baseUrl);
    void downloadFile(Users user, Long fileId, HttpServletResponse response) throws Exception;
    void downloadSharedFile(Long fileId, String token, HttpServletResponse response) throws Exception;
    void previewFile(Users user, Long fileId, HttpServletResponse response) throws Exception;
    void streamReferencedFile(Long fileId, HttpServletResponse response, boolean inline) throws Exception;
}
