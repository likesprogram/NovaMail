package com.example.novamailservice.controller.Users;

import com.example.novamailservice.dto.MailBoxAddressToMailBoxAndUser;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.HistoryAvatar;
import com.example.novamailservice.dto.Users.ResultUserProfile;
import com.example.novamailservice.entity.Users.UserLoginLog;
import com.example.novamailservice.entity.Users.UserProfile;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.Mail.MailBoxesService;
import com.example.novamailservice.service.User.*;
import io.swagger.v3.oas.annotations.Operation;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/userProfile")
public class UserProfileController {
    private final UserAvatarService userAvatarService;
    private final UserProfileService userProfileService;
    private final MailBoxesService mailBoxesService;
    private final UsersService usersService;
    private final UserLoginLogService userLoginLogService;

    @Autowired
    public UserProfileController(UserAvatarService userAvatarService, UserProfileService userProfileService, MailBoxesService mailBoxesService, UsersService usersService, UserLoginLogService userLoginLogService) {
        this.userAvatarService = userAvatarService;
        this.userProfileService = userProfileService;
        this.mailBoxesService = mailBoxesService;
        this.usersService = usersService;
        this.userLoginLogService = userLoginLogService;
    }

    @PostMapping("/getCurrentInfo")
    public ResponseResult<ResultUserProfile> getCurrentInfo() {
        ResultUserProfile resultUserProfile = new ResultUserProfile();

        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        // 获取账号信息
        ResponseResult<UserProfile> userProfile = userProfileService.getUserProfileByAccount(currentUser.getData().getAccount());
        if (!userProfile.isSuccess()) return ResponseResult.error(ErrorCode.NOT_FOUND, "信息不存在");

        resultUserProfile.setUserId(currentUser.getData().getUserId());
        resultUserProfile.setAccount(currentUser.getData().getAccount());
        resultUserProfile.setIsAdmin(currentUser.getData().getIsAdmin());
        resultUserProfile.setStatus(currentUser.getData().getStatus());

        resultUserProfile.setNickname(userProfile.getData().getNickname());
        resultUserProfile.setAvatar(userProfile.getData().getAvatar());
        resultUserProfile.setAddress(userProfile.getData().getAddress());
        resultUserProfile.setResume(userProfile.getData().getResume());

        // 获取邮箱地址
        Long mailboxId = userProfile.getData().getDefaultMailBoxId();
        resultUserProfile.setDefaultMailBoxId(mailboxId);
        if (mailboxId != null && mailboxId > 0) {
            ResponseResult<String> mailResult = mailBoxesService.getMailBoxAddress(mailboxId);
            if (mailResult.isSuccess()) resultUserProfile.setDefaultMailBoxView(mailResult.getData());
        }

        resultUserProfile.setUpdateTime(userProfile.getData().getUpdateTime());
        resultUserProfile.setRegisterTime(currentUser.getData().getRegisterTime());
        resultUserProfile.setLastLoginTime(currentUser.getData().getLastLoginTime());

        return ResponseResult.success(resultUserProfile);
    }

    @GetMapping("/avatar")
    @Operation(summary = "用户头像")
    public void avatar(@RequestParam("account") String account, HttpServletResponse response) throws Exception {
        userAvatarService.writeAvatarByAccount(account, response);
    }

    @GetMapping("/mailBoxAvatar")
    @Operation(summary = "用户头像")
    public void mailBoxAvatar(@RequestParam("mailBoxAddress") String mailBoxAddress, HttpServletResponse response) throws Exception {
        ResponseResult<MailBoxAddressToMailBoxAndUser> account = mailBoxesService.getAccountByMailBoxAddress(mailBoxAddress);
        if (!account.isSuccess()) userAvatarService.writeDefaultAvatar(response);
        else userAvatarService.writeAvatarByAccount(account.getData().getUser().getAccount(), response);
    }

    @PostMapping("/getHistoryAvatars")
    public ResponseResult<List<HistoryAvatar>> getHistoryAvatars() {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return userProfileService.getUserHistoryAvatars(currentUser.getData().getAccount());
    }

    @PostMapping("/updateNickname")
    public ResponseResult<String> updateNickname(@RequestBody Map<String, String> body) {
        String nickname = body.get("nickname");
        if (nickname == null || nickname.isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return userProfileService.updateNickname(currentUser.getData(), nickname);
    }

    @PostMapping("/updateAvatar")
    public ResponseResult<String> updateAvatar(@RequestBody Map<String, String> body) {
        // 参数验证
        String base64Data = body.get("base64"), fileName = body.get("fileName");
        boolean hasBase64 = base64Data != null && !base64Data.isEmpty();
        boolean hasFileName = fileName != null && !fileName.isEmpty();
        if (hasBase64 == hasFileName) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        try {
            return userProfileService.updateAvatar(currentUser.getData(), base64Data, fileName);
        } catch (Exception e) {
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "头像更新失败");
        }
    }

    @PostMapping("/updateResume")
    public ResponseResult<String> updateResume(@RequestBody Map<String, String> body) {
        String resume = body.get("resume");
        if (resume == null || resume.isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return userProfileService.updateResume(currentUser.getData(), resume);
    }

    @PostMapping("/getLoginLogs")
    public ResponseResult<List<UserLoginLog>> getLoginLogs(@RequestBody Map<String, String> body) {
        String account = body.get("account");
        if (account == null || account.isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        // 用户 ID 不一致，且不是管理员
        if (!account.equals(currentUser.getData().getAccount()) && currentUser.getData().getIsAdmin())
            return ResponseResult.error(ErrorCode.FORBIDDEN, "你无法获取他人数据");

        // 拉取 100 行登录日志
        try {
            List<UserLoginLog> userLoginLogs = userLoginLogService.select(account, 100L);
            return ResponseResult.success("获取成功", userLoginLogs);
        } catch (Exception e) {
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "登录日志获取失败");
        }
    }
}
