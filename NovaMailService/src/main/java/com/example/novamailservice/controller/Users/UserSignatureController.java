package com.example.novamailservice.controller.Users;

import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.CreateUserSignature;
import com.example.novamailservice.entity.Users.UserSignature;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.User.UserSignatureService;
import com.example.novamailservice.service.User.UsersService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/userSignature")
public class UserSignatureController {
    private final UserSignatureService userSignatureService;
    private final UsersService usersService;

    @Autowired
    public UserSignatureController(UserSignatureService userSignatureService, UsersService usersService) {
        this.userSignatureService = userSignatureService;
        this.usersService = usersService;
    }

    @PostMapping("/createSignature")
    public ResponseResult<String> createSignature(@RequestBody CreateUserSignature newUserSignature) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        UserSignature userSignature = new UserSignature();
        userSignature.setUserId(currentUser.getData().getUserId());
        userSignature.setSignatureName(newUserSignature.getSignatureName());
        userSignature.setSignatureContent(newUserSignature.getSignatureContent());

        return userSignatureService.createSignature(userSignature);
    }

    @PostMapping("/getDefaultSignature")
    public ResponseResult<UserSignature> getDefaultSignature() {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return userSignatureService.getDefaultSignature(currentUser.getData().getUserId());
    }

    @PostMapping("/setDefaultSignature")
    public ResponseResult<String> setDefaultSignature(@RequestBody UserSignature userSignature) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        // 用户 ID 不一致，且不是管理员
        if (!userSignature.getUserId().equals(currentUser.getData().getUserId()) && currentUser.getData().getIsAdmin())
            return ResponseResult.error(ErrorCode.FORBIDDEN, "你无法修改他人数据");

        if (userSignatureService.setDefaultSignature(userSignature.getSignatureId(), currentUser.getData().getUserId(), userSignature.isDefault())) return ResponseResult.success("设置成功");
        return ResponseResult.error("设置失败");
    }

    @PostMapping("/getSignatures")
    public ResponseResult<List<UserSignature>> getUserSignatures() {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return userSignatureService.getSignatures(currentUser.getData().getUserId());
    }

    @PostMapping("/getSignatureContent")
    public ResponseResult<String> getUserSignatureContent(@RequestBody Map<String, Long> body) {
        Long signatureId = body.get("signatureId");
        if (signatureId == null || signatureId == 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return userSignatureService.getSignature(signatureId);
    }

    @PostMapping("/updateSignature")
    public ResponseResult<String> updateSignature(@RequestBody UserSignature userSignature) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        // 用户 ID 不一致，且不是管理员
        if (!userSignature.getUserId().equals(currentUser.getData().getUserId()) && currentUser.getData().getIsAdmin())
            return ResponseResult.error(ErrorCode.FORBIDDEN, "你无法修改他人数据");

        if (userSignatureService.updateSignature(userSignature)) return ResponseResult.success("修改成功");
        return ResponseResult.error("修改失败");
    }

    @PostMapping("/deleteSignature")
    public ResponseResult<String> deleteSignature(@RequestBody UserSignature userSignature) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        // 用户 ID 不一致，且不是管理员
        if (!userSignature.getUserId().equals(currentUser.getData().getUserId()) && currentUser.getData().getIsAdmin())
            return ResponseResult.error(ErrorCode.FORBIDDEN, "你无法修改他人数据");

        if (userSignatureService.deleteSignature(userSignature)) return ResponseResult.success("删除成功");
        return ResponseResult.error("删除失败");
    }
}
