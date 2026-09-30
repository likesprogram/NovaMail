package com.example.novamailservice.controller.Users;

import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.AddressBookData;
import com.example.novamailservice.dto.Users.SaveContactGroupRequest;
import com.example.novamailservice.dto.Users.SaveContactRequest;
import com.example.novamailservice.entity.Users.Contact;
import com.example.novamailservice.entity.Users.ContactGroup;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.service.User.AddressBookService;
import com.example.novamailservice.service.User.UsersService;
import io.swagger.v3.oas.annotations.Operation;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.Map;

@RestController
@RequestMapping("/addressBook")
public class AddressBookController {
    private final UsersService usersService;
    private final AddressBookService addressBookService;

    public AddressBookController(UsersService usersService, AddressBookService addressBookService) {
        this.usersService = usersService;
        this.addressBookService = addressBookService;
    }

    @PostMapping("/get")
    @Operation(summary = "获取通讯录")
    public ResponseResult<AddressBookData> getAddressBook() {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return addressBookService.getAddressBook(currentUser.getData());
    }

    @PostMapping("/saveGroup")
    @Operation(summary = "保存通讯录分组")
    public ResponseResult<ContactGroup> saveGroup(@RequestBody SaveContactGroupRequest request) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return addressBookService.saveGroup(currentUser.getData(), request);
    }

    @PostMapping("/deleteGroup")
    @Operation(summary = "删除通讯录分组")
    public ResponseResult<String> deleteGroup(@RequestBody Map<String, Long> request) {
        Long groupId = request == null ? null : request.get("groupId");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return addressBookService.deleteGroup(currentUser.getData(), groupId);
    }

    @PostMapping("/saveContact")
    @Operation(summary = "保存通讯录联系人")
    public ResponseResult<Contact> saveContact(@RequestBody SaveContactRequest request) {
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return addressBookService.saveContact(currentUser.getData(), request);
    }

    @PostMapping("/deleteContact")
    @Operation(summary = "删除通讯录联系人")
    public ResponseResult<String> deleteContact(@RequestBody Map<String, Long> request) {
        Long contactId = request == null ? null : request.get("contactId");
        ResponseResult<Users> currentUser = usersService.getCurrentUser();
        if (!currentUser.isSuccess()) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        return addressBookService.deleteContact(currentUser.getData(), contactId);
    }
}
