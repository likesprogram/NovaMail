package com.example.novamailservice.controller.System;

import com.example.novamailservice.dto.DomainNames.DomainDkimConfig;
import com.example.novamailservice.dto.DomainNames.DomainDnsVerification;
import com.example.novamailservice.dto.DomainNames.ResultDomainName;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.System.SystemClientDownloadInfo;
import com.example.novamailservice.dto.System.SystemServiceMonitorInfo;
import com.example.novamailservice.entity.DomainNames.DomainCredentials;
import com.example.novamailservice.entity.DomainNames.DomainNames;
import com.example.novamailservice.entity.System.SystemSettings;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.DomainNames.DomainNamesService;
import com.example.novamailservice.service.System.SystemServiceMonitorService;
import com.example.novamailservice.service.System.SystemSettingsService;
import com.example.novamailservice.service.User.UsersService;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.multipart.MultipartFile;

import java.util.ArrayList;
import java.util.List;

@RestController
@RequestMapping("/system")
public class SystemController {
    private final UsersService usersService;
    private final DomainNamesService domainNamesService;
    private final SystemSettingsService systemSettingsService;
    private final SystemServiceMonitorService systemServiceMonitorService;

    @Autowired
    public SystemController(UsersService usersService, DomainNamesService domainNamesService, SystemSettingsService systemSettingsService, SystemServiceMonitorService systemServiceMonitorService) {
        this.usersService = usersService;
        this.domainNamesService = domainNamesService;
        this.systemSettingsService = systemSettingsService;
        this.systemServiceMonitorService = systemServiceMonitorService;
    }

    private ResponseResult<Users> requireAdmin() {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (!Boolean.TRUE.equals(currentUser.getData().getIsAdmin())) {
            return ResponseResult.error(ErrorCode.FORBIDDEN, "需要管理员权限");
        }
        return currentUser;
    }

    @PostMapping("/getDomainList")
    public ResponseResult<List<ResultDomainName>> getDomainList() {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        ResponseResult<List<DomainNames>> domainResult = domainNamesService.getAllDomainNames();
        if (!domainResult.isSuccess()) return ResponseResult.error(domainResult.getCode(), domainResult.getMessage());

        List<ResultDomainName> result = new ArrayList<>();
        for (DomainNames domainName : domainResult.getData()) {
            ResultDomainName item = ResultDomainName.fromDomainName(domainName, currentUser.getData().getUserId(), "SYSTEM");
            item.setEditable(true);
            result.add(item);
        }
        return ResponseResult.success(result);
    }

    @PostMapping("/createDomain")
    public ResponseResult<ResultDomainName> createDomain(@RequestBody DomainNames domainName) {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        ResponseResult<DomainNames> created = domainNamesService.createSystemDomainName(domainName);
        if (!created.isSuccess()) return ResponseResult.error(created.getCode(), created.getMessage());

        ResultDomainName result = ResultDomainName.fromDomainName(created.getData(), currentUser.getData().getUserId(), "SYSTEM");
        result.setEditable(true);
        return ResponseResult.success(result);
    }

    @PostMapping("/updateDomain")
    public ResponseResult<ResultDomainName> updateDomain(@RequestBody DomainNames domainName) {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        ResponseResult<DomainNames> updated = domainNamesService.updateSystemDomainName(domainName);
        if (!updated.isSuccess()) return ResponseResult.error(updated.getCode(), updated.getMessage());

        ResultDomainName result = ResultDomainName.fromDomainName(updated.getData(), currentUser.getData().getUserId(), "SYSTEM");
        result.setEditable(true);
        return ResponseResult.success(result);
    }

    @PostMapping("/deleteDomain")
    public ResponseResult<String> deleteDomain(@RequestBody DomainNames domainName) {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        return domainNamesService.deleteSystemDomainName(domainName.getDomainNameId());
    }

    @PostMapping("/setDefaultDomain")
    public ResponseResult<String> setDefaultDomain(@RequestBody DomainNames domainName) {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        return domainNamesService.setSystemDefaultDomainName(domainName.getDomainNameId());
    }

    @PostMapping("/getDomainDkimConfig")
    public ResponseResult<DomainDkimConfig> getDomainDkimConfig(@RequestBody DomainNames domainName) {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        return domainNamesService.getSystemDomainDkimConfig(domainName.getDomainNameId());
    }

    @PostMapping("/configureDomainTls")
    public ResponseResult<String> configureDomainTls(@RequestBody DomainCredentials domainCredentials) {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        return domainNamesService.configureSystemDomainTls(domainCredentials);
    }

    @PostMapping("/verifyDomainDns")
    public ResponseResult<DomainDnsVerification> verifyDomainDns(@RequestBody DomainNames domainName) {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        return domainNamesService.verifySystemDomainDns(domainName.getDomainNameId());
    }

    @PostMapping("/getDefaultAvatar")
    public ResponseResult<SystemSettings> getDefaultAvatar() {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        return systemSettingsService.getDefaultAvatar();
    }

    @PostMapping("/setDefaultAvatar")
    public ResponseResult<SystemSettings> setDefaultAvatar(@RequestBody SystemSettings systemSettings) {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());
        if (systemSettings == null || systemSettings.getSettingValue() == null || systemSettings.getSettingValue().isBlank()) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "默认头像不能为空");
        }

        if (!systemSettingsService.setDefaultAvatar(systemSettings.getSettingValue())) {
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "默认头像保存失败");
        }
        return systemSettingsService.getDefaultAvatar();
    }

    @PostMapping("/uploadDefaultAvatar")
    public ResponseResult<SystemSettings> uploadDefaultAvatar(@RequestParam("file") MultipartFile file) {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        return systemSettingsService.uploadDefaultAvatar(file);
    }

    @PostMapping("/getDefaultSetting")
    public ResponseResult<SystemSettings> getDefaultSetting() {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        return systemSettingsService.getDefaultSetting();
    }

    @PostMapping("/setDefaultSetting")
    public ResponseResult<SystemSettings> setDefaultSetting(@RequestBody SystemSettings systemSettings) {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());
        if (systemSettings == null || systemSettings.getSettingValue() == null || systemSettings.getSettingValue().isBlank()) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "默认资料卡样式不能为空");
        }

        if (!systemSettingsService.setDefaultSetting(systemSettings.getSettingValue())) {
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "默认资料卡样式保存失败");
        }
        return systemSettingsService.getDefaultSetting();
    }

    @PostMapping("/uploadClientPackage")
    public ResponseResult<SystemClientDownloadInfo> uploadClientPackage(
            @RequestParam("file") MultipartFile file,
            @RequestParam(value = "version", required = false) String version,
            @RequestParam(value = "description", required = false) String description
    ) {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        return systemSettingsService.uploadClientPackage(file, version, description);
    }

    @PostMapping("/getClientDownloadInfo")
    public ResponseResult<SystemClientDownloadInfo> getClientDownloadInfo() {
        return systemSettingsService.getClientDownloadInfo();
    }

    @RequestMapping("/downloadClient")
    public void downloadClient(HttpServletResponse response) throws Exception {
        systemSettingsService.downloadClientPackage(response);
    }

    @PostMapping("/getServiceMonitorList")
    public ResponseResult<List<SystemServiceMonitorInfo>> getServiceMonitorList() {
        ResponseResult<Users> currentUser = requireAdmin();
        if (!currentUser.isSuccess()) return ResponseResult.error(currentUser.getCode(), currentUser.getMessage());

        return systemServiceMonitorService.getServiceMonitorList();
    }
}
