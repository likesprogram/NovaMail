package com.example.novamailservice.controller.MailBox;

import com.example.novamailservice.dto.Mail.*;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.Mail.EmailService;
import com.example.novamailservice.service.User.UsersService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;
import io.swagger.v3.oas.annotations.Operation;

import java.util.Collections;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/email")
public class EmailController {
    private final UsersService usersService;
    private final EmailService emailService;

    @Autowired
    public EmailController(UsersService usersService, EmailService emailService) {
        this.usersService = usersService;
        this.emailService = emailService;
    }

    @PostMapping("/sendMail")
    @Operation(summary = "发送邮件")
    public ResponseResult<String> sendMail(@RequestBody SendMailRequest request) {
        if (request == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "请求参数不能为空");
        return emailService.sendMail(request.getMailBoxId(), request.getSendName(), request.getToAddressList(), request.getSubject(), request.getBodyData());
    }

    @PostMapping("/getFolderList")
    public ResponseResult<List<RestMailFolder>> getFolderList() {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return ResponseResult.success(emailService.getMailFolder(currentUser.getData()));
    }

    @PostMapping("/getEmailList")
    public ResponseResult<List<MailBoxMailMapping>> getEmailList(@RequestBody RestMailFolder restMailFolder) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return ResponseResult.success(emailService.getEmailList(currentUser.getData(), restMailFolder));
    }

    @PostMapping("/getMailDetail")
    public ResponseResult<MailDetailResponse> getMailDetail(@RequestBody Map<String, Long> request) {
        Long mappingId = request.get("mappingId");
        if (mappingId == null || mappingId == 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        MailDetailResponse detail = emailService.getMailDetail(currentUser.getData(), mappingId);
        return ResponseResult.success(detail);
    }

    @PostMapping("/setRead")
    public ResponseResult<Void> setRead(@RequestBody UpdateStatus updateStatus) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        emailService.setMailReader(currentUser.getData(), updateStatus.getMappingId(), updateStatus.isData());
        return ResponseResult.success();
    }

    @PostMapping("/setStarred")
    public ResponseResult<Void> setStarred(@RequestBody UpdateStatus updateStatus) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        emailService.setStarred(currentUser.getData(), updateStatus.getMappingId(), updateStatus.isData());
        return ResponseResult.success();
    }

    @PostMapping("/move")
    public ResponseResult<Void> move(@RequestBody MailBoxMailMapping mailBoxMailMapping) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        return ResponseResult.error();
    }

    @PostMapping("/notrash")
    public ResponseResult<Void> notrash(@RequestBody MailBoxMailMapping mailBoxMailMapping) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        return ResponseResult.error();
    }

    @PostMapping("/noadvertisement")
    public ResponseResult<Void> noadvertisement(@RequestBody MailBoxMailMapping mailBoxMailMapping) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        return ResponseResult.error();
    }

    @GetMapping("/search/suggest")
    public ResponseResult<List<SearchSuggestionVO>> suggest(@RequestParam("keyword") String keyword, @RequestParam(name = "limit", defaultValue = "8") Integer limit) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (keyword == null || keyword.trim().isEmpty()) return ResponseResult.success(Collections.emptyList());
        List<SearchSuggestionVO> result = emailService.getSearchSuggestions(currentUser.getData(), keyword.trim(), limit);
        return ResponseResult.success(result);
    }
}
