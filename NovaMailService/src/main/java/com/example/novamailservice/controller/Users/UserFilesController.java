package com.example.novamailservice.controller.Users;

import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.UpdateUserFileRequest;
import com.example.novamailservice.dto.Users.UserFileAttachmentAuthorization;
import com.example.novamailservice.entity.Users.UserFiles;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.User.UserFilesService;
import com.example.novamailservice.service.User.UsersService;
import io.swagger.v3.oas.annotations.Operation;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.multipart.MultipartFile;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/userFiles")
public class UserFilesController {
    private final UsersService usersService;
    private final UserFilesService userFilesService;

    public UserFilesController(UsersService usersService, UserFilesService userFilesService) {
        this.usersService = usersService;
        this.userFilesService = userFilesService;
    }

    @PostMapping("/list")
    @Operation(summary = "获取用户文件列表")
    public ResponseResult<List<UserFiles>> list() {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return userFilesService.listFiles(currentUser.getData());
    }

    @PostMapping("/upload")
    @Operation(summary = "上传用户文件")
    public ResponseResult<UserFiles> upload(
            @RequestParam("file") MultipartFile file,
            @RequestParam(value = "filePath", required = false) String filePath,
            @RequestParam(value = "description", required = false) String description
    ) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return userFilesService.uploadFile(currentUser.getData(), file, filePath, description);
    }

    @PostMapping("/update")
    @Operation(summary = "更新用户文件信息")
    public ResponseResult<UserFiles> update(@RequestBody UpdateUserFileRequest request) {
        if (request == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return userFilesService.updateFile(currentUser.getData(), request.getFileId(), request.getFileName(), request.getDescription());
    }

    @PostMapping("/delete")
    @Operation(summary = "删除用户文件")
    public ResponseResult<String> delete(@RequestBody Map<String, Long> request) {
        Long fileId = request == null ? null : request.get("fileId");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return userFilesService.deleteFile(currentUser.getData(), fileId);
    }

    @PostMapping("/attachmentAuthorizations")
    @Operation(summary = "获取用户文件附件授权列表")
    public ResponseResult<List<UserFileAttachmentAuthorization>> attachmentAuthorizations(@RequestBody Map<String, Long> request) {
        Long fileId = request == null ? null : request.get("fileId");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return userFilesService.listAttachmentAuthorizations(currentUser.getData(), fileId);
    }

    @PostMapping("/revokeAttachmentAuthorization")
    @Operation(summary = "撤销用户文件附件授权")
    public ResponseResult<String> revokeAttachmentAuthorization(@RequestBody Map<String, Long> request) {
        Long fileId = request == null ? null : request.get("fileId");
        Long attachmentId = request == null ? null : request.get("attachmentId");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return userFilesService.revokeAttachmentAuthorization(currentUser.getData(), fileId, attachmentId);
    }

    @GetMapping("/download")
    @Operation(summary = "下载用户文件")
    public void download(@RequestParam("fileId") Long fileId, HttpServletResponse response) throws Exception {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        userFilesService.downloadFile(currentUser.getData(), fileId, response);
    }

    @GetMapping("/shared/download")
    public void sharedDownload(
            @RequestParam("fileId") Long fileId,
            @RequestParam("token") String token,
            HttpServletResponse response
    ) throws Exception {
        userFilesService.downloadSharedFile(fileId, token, response);
    }

    @GetMapping("/preview")
    @Operation(summary = "预览用户文件")
    public void preview(@RequestParam("fileId") Long fileId, HttpServletResponse response) throws Exception {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }
        userFilesService.previewFile(currentUser.getData(), fileId, response);
    }
}
