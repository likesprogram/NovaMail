package com.example.novamailservice.controller.MailBox;

import com.example.novamailservice.dto.Mail.ResultMailBoxes;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.DomainNames.DomainNames;
import com.example.novamailservice.entity.Mail.MailBoxes;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.DomainNames.DomainNamesService;
import com.example.novamailservice.service.Mail.MailBoxesService;
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

    @Autowired
    public MailBoxController(UsersService usersService, DomainNamesService domainNamesService, MailBoxesService mailBoxesService) {
        this.usersService = usersService;
        this.domainNamesService = domainNamesService;
        this.mailBoxesService = mailBoxesService;
    }

    @PostMapping("/createMailBox")
    public ResponseResult<String> createMailBox(@RequestBody MailBoxes mailBox) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        ResponseResult<DomainNames> domainName = domainNamesService.getDomainNameById(mailBox.getDomainNameId());
        if (!domainName.isSuccess()) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");

        // 当前域名不是公共域名，且不是该用户所有
        Long ownerUserId = domainName.getData().getUserId();
        Long currentUserId = currentUser.getData().getUserId();
        if (ownerUserId != null && !ownerUserId.equals(currentUserId)) {
            // 查看授权列表
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

    @PostMapping("/setOpenMailBox")
    public ResponseResult<String> openMailBox(@RequestBody MailBoxes mailBox) {
        // 获取当前登录账号
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");

        // 验证邮箱所属
        if (!currentUser.getData().getUserId().equals(mailBox.getUserId())) return ResponseResult.error(ErrorCode.FORBIDDEN, "你无权操作此邮箱");

//        if (mailBoxesService.openMailBox(mailBox)) {
//            return ResponseResult.success("操作成功");
//        }
        return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "操作失败");
    }
}
