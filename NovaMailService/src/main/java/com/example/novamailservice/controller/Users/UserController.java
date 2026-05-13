package com.example.novamailservice.controller.Users;

import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.UpdatePassword;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.Authentication.AuthenticationService;
import com.example.novamailservice.service.User.UsersService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/user")
public class UserController {
    private final UsersService usersService;
    private final AuthenticationService authenticationService;

    @Autowired
    public UserController(UsersService usersService, AuthenticationService authenticationService) {
        this.usersService = usersService;
        this.authenticationService = authenticationService;
    }

    @PostMapping("/updatePassword")
    public ResponseResult<String> updatePassword(@RequestBody UpdatePassword newPassword) {
        if ( // 参数验证
            newPassword.getOldPassword() == null || newPassword.getOldPassword().isEmpty() ||
            newPassword.getNewPassword() == null || newPassword.getNewPassword().isEmpty() ||
            newPassword.getConfirmPassword() == null || newPassword.getConfirmPassword().isEmpty()
        ) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        // 修改密码
        ResponseResult<String> result = usersService.updatePassword(currentUser.getData(), newPassword);
        if (result.isSuccess()) authenticationService.logout(); // 移除登录信息

        return result;
    }
}
