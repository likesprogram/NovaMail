package com.example.novamailservice.controller.MailBox;

import com.example.novamailservice.dto.DomainNames.DomainDkimConfig;
import com.example.novamailservice.dto.DomainNames.DomainDnsVerification;
import com.example.novamailservice.dto.DomainNames.ResultDomainName;
import com.example.novamailservice.dto.Mail.ResultMailBoxes;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.DomainNames.DomainCredentials;
import com.example.novamailservice.entity.DomainNames.DomainNames;
import com.example.novamailservice.entity.Mail.EmailAuthorization;
import com.example.novamailservice.entity.Mail.MailBoxes;
import com.example.novamailservice.entity.Users.UserProfile;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.DomainNames.DomainNamesService;
import com.example.novamailservice.service.Mail.EmailAuthorizationService;
import com.example.novamailservice.service.Mail.MailBoxesService;
import com.example.novamailservice.service.User.UserProfileService;
import com.example.novamailservice.service.User.UsersService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.ArrayList;
import java.util.List;

@RestController
@RequestMapping("/mailBox")
public class MailBoxController {
    private final UsersService usersService;
    private final DomainNamesService domainNamesService;
    private final MailBoxesService mailBoxesService;
    private final UserProfileService userProfileService;
    private final EmailAuthorizationService emailAuthorizationService;

    @Autowired
    public MailBoxController(UsersService usersService, DomainNamesService domainNamesService, MailBoxesService mailBoxesService, UserProfileService userProfileService, EmailAuthorizationService emailAuthorizationService) {
        this.usersService = usersService;
        this.domainNamesService = domainNamesService;
        this.mailBoxesService = mailBoxesService;
        this.userProfileService = userProfileService;
        this.emailAuthorizationService = emailAuthorizationService;
    }

    @PostMapping("/createMailBox")
    public ResponseResult<String> createMailBox(@RequestBody MailBoxes mailBox) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        ResponseResult<DomainNames> domainName = domainNamesService.getDomainNameById(mailBox.getDomainNameId());
        if (!domainName.isSuccess()) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");

        Long currentUserId = currentUser.getData().getUserId();
        if (!domainNamesService.canUseDomain(domainName.getData(), currentUserId)) {
            return ResponseResult.error(ErrorCode.FORBIDDEN, "你无权使用这个域名");
        }

        // 创建邮箱
        try {
            ResponseResult<MailBoxes> mailBoxesResponseResult = mailBoxesService.createMailBox(currentUser.getData(), domainName.getData(), mailBox.getLocalPart(), 0);
            if (!mailBoxesResponseResult.isSuccess()) {
                return ResponseResult.error(mailBoxesResponseResult.getCode(), mailBoxesResponseResult.getMessage());
            }
            return ResponseResult.success("邮箱创建成功");
        } catch (Exception e) {
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "邮箱创建失败");
        }
    }

    @PostMapping("/getMailBoxList")
    public ResponseResult<List<ResultMailBoxes>> getMailBoxList() {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        List<ResultMailBoxes> resultMailBoxes = new ArrayList<>();
        ResponseResult<List<MailBoxes>> listResponseResult = mailBoxesService.getMailBoxListByUserId(currentUser.getData().getUserId());
        if (!listResponseResult.isSuccess()) return ResponseResult.error(listResponseResult.getCode(), listResponseResult.getMessage());

        // 类型转换
        for (MailBoxes mailBox : listResponseResult.getData()) {
            ResponseResult<DomainNames> domainName = domainNamesService.getDomainNameById(mailBox.getDomainNameId());
            if (!domainName.isSuccess()) continue;
            resultMailBoxes.add(ResultMailBoxes.MailBoxesToResultMailBoxes(mailBox, domainName.getData()));
        }
        return ResponseResult.success(resultMailBoxes);
    }

    @PostMapping("/getAvailableDomainList")
    public ResponseResult<List<ResultDomainName>> getAvailableDomainList() {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        Long currentUserId = currentUser.getData().getUserId();
        ResponseResult<List<DomainNames>> domainResult = domainNamesService.getAvailableDomainNameByUserId(currentUserId);
        if (!domainResult.isSuccess()) return ResponseResult.error(domainResult.getCode(), domainResult.getMessage());

        List<ResultDomainName> result = new ArrayList<>();
        for (DomainNames domainName : domainResult.getData()) {
            String permission = domainNamesService.getUserDomainPermission(domainName, currentUserId);
            result.add(ResultDomainName.fromDomainName(domainName, currentUserId, permission));
        }
        return ResponseResult.success(result);
    }

    @PostMapping("/createDomain")
    public ResponseResult<ResultDomainName> createDomain(@RequestBody DomainNames domainName) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        Long currentUserId = currentUser.getData().getUserId();
        ResponseResult<DomainNames> created = domainNamesService.createUserDomainName(currentUserId, domainName);
        if (!created.isSuccess()) return ResponseResult.error(created.getCode(), created.getMessage());

        return ResponseResult.success(ResultDomainName.fromDomainName(created.getData(), currentUserId, "OWNER"));
    }

    @PostMapping("/updateDomain")
    public ResponseResult<ResultDomainName> updateDomain(@RequestBody DomainNames domainName) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        Long currentUserId = currentUser.getData().getUserId();
        ResponseResult<DomainNames> updated = domainNamesService.updateUserDomainName(currentUserId, domainName);
        if (!updated.isSuccess()) return ResponseResult.error(updated.getCode(), updated.getMessage());

        return ResponseResult.success(ResultDomainName.fromDomainName(updated.getData(), currentUserId, "OWNER"));
    }

    @PostMapping("/deleteDomain")
    public ResponseResult<String> deleteDomain(@RequestBody DomainNames domainName) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return domainNamesService.deleteUserDomainName(currentUser.getData().getUserId(), domainName.getDomainNameId());
    }

    @PostMapping("/getDomainDkimConfig")
    public ResponseResult<DomainDkimConfig> getDomainDkimConfig(@RequestBody DomainNames domainName) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return domainNamesService.getUserDomainDkimConfig(currentUser.getData().getUserId(), domainName.getDomainNameId());
    }

    @PostMapping("/configureDomainTls")
    public ResponseResult<String> configureDomainTls(@RequestBody DomainCredentials domainCredentials) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return domainNamesService.configureUserDomainTls(currentUser.getData().getUserId(), domainCredentials);
    }

    @PostMapping("/verifyDomainDns")
    public ResponseResult<DomainDnsVerification> verifyDomainDns(@RequestBody DomainNames domainName) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return domainNamesService.verifyUserDomainDns(currentUser.getData().getUserId(), domainName.getDomainNameId());
    }

    @PostMapping("/setOpenMailBox")
    public ResponseResult<String> openMailBox(@RequestBody MailBoxes mailBox) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        if (mailBox.getMailBoxId() == null || mailBox.getIsOpen() == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        if (mailBox.getIsOpen() != 0 && mailBox.getIsOpen() != 1) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        Long currentUserId = currentUser.getData().getUserId();
        ResponseResult<MailBoxes> existing = mailBoxesService.getMailBoxById(mailBox.getMailBoxId());
        if (!existing.isSuccess()) return ResponseResult.error(existing.getCode(), existing.getMessage());
        if (!currentUserId.equals(existing.getData().getUserId())) return ResponseResult.error(ErrorCode.FORBIDDEN, "你无权操作此邮箱");
        if (mailBox.getIsOpen() == 0) {
            ResponseResult<UserProfile> profile = userProfileService.getUserProfileByAccount(currentUser.getData().getAccount());
            if (profile.isSuccess() && mailBox.getMailBoxId().equals(profile.getData().getDefaultMailBoxId())) {
                return ResponseResult.error(ErrorCode.FORBIDDEN, "默认邮箱不能停用");
            }
        }

        return mailBoxesService.updateMailBoxOpenStatus(currentUserId, mailBox.getMailBoxId(), mailBox.getIsOpen());
    }

    @PostMapping("/setDefaultMailBox")
    public ResponseResult<String> setDefaultMailBox(@RequestBody MailBoxes mailBox) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (mailBox.getMailBoxId() == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        Long currentUserId = currentUser.getData().getUserId();
        ResponseResult<MailBoxes> existing = mailBoxesService.getMailBoxById(mailBox.getMailBoxId());
        if (!existing.isSuccess()) return ResponseResult.error(existing.getCode(), existing.getMessage());
        if (!currentUserId.equals(existing.getData().getUserId())) return ResponseResult.error(ErrorCode.FORBIDDEN, "你无权操作此邮箱");
        if (!Integer.valueOf(1).equals(existing.getData().getIsOpen())) return ResponseResult.error(ErrorCode.FORBIDDEN, "停用邮箱不能设为默认");

        return userProfileService.updateDefaultMailBox(currentUser.getData(), mailBox.getMailBoxId());
    }

    @PostMapping("/removeMailBox")
    public ResponseResult<String> removeMailBox(@RequestBody MailBoxes mailBox) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (mailBox.getMailBoxId() == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        Long currentUserId = currentUser.getData().getUserId();
        ResponseResult<MailBoxes> existing = mailBoxesService.getMailBoxById(mailBox.getMailBoxId());
        if (!existing.isSuccess()) return ResponseResult.error(existing.getCode(), existing.getMessage());
        if (!currentUserId.equals(existing.getData().getUserId())) return ResponseResult.error(ErrorCode.FORBIDDEN, "你无权操作此邮箱");
        ResponseResult<UserProfile> profile = userProfileService.getUserProfileByAccount(currentUser.getData().getAccount());
        if (profile.isSuccess() && mailBox.getMailBoxId().equals(profile.getData().getDefaultMailBoxId())) {
            return ResponseResult.error(ErrorCode.FORBIDDEN, "默认邮箱不能移除");
        }

        return mailBoxesService.updateMailBoxOpenStatus(currentUserId, mailBox.getMailBoxId(), 2);
    }

    @PostMapping("/getEmailAuthorizationList")
    public ResponseResult<List<EmailAuthorization>> getEmailAuthorizationList() {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return emailAuthorizationService.getAuthorizationList(currentUser.getData().getUserId());
    }

    @PostMapping("/createEmailAuthorization")
    public ResponseResult<EmailAuthorization> createEmailAuthorization(@RequestBody EmailAuthorization emailAuthorization) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return emailAuthorizationService.createAuthorization(currentUser.getData().getUserId(), emailAuthorization);
    }

    @PostMapping("/updateEmailAuthorization")
    public ResponseResult<EmailAuthorization> updateEmailAuthorization(@RequestBody EmailAuthorization emailAuthorization) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return emailAuthorizationService.updateAuthorization(currentUser.getData().getUserId(), emailAuthorization);
    }

    @PostMapping("/setEmailAuthorizationActivated")
    public ResponseResult<String> setEmailAuthorizationActivated(@RequestBody EmailAuthorization emailAuthorization) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return emailAuthorizationService.updateAuthorizationActivated(
                currentUser.getData().getUserId(),
                emailAuthorization.getEmailAuthorizationId(),
                emailAuthorization.getIsActivated()
        );
    }

    @PostMapping("/deleteEmailAuthorization")
    public ResponseResult<String> deleteEmailAuthorization(@RequestBody EmailAuthorization emailAuthorization) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        return emailAuthorizationService.deleteAuthorization(
                currentUser.getData().getUserId(),
                emailAuthorization.getEmailAuthorizationId()
        );
    }
}
