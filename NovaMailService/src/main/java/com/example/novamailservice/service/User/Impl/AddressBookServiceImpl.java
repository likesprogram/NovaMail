package com.example.novamailservice.service.User.Impl;

import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.AddressBookData;
import com.example.novamailservice.dto.Users.SaveContactGroupRequest;
import com.example.novamailservice.dto.Users.SaveContactRequest;
import com.example.novamailservice.entity.Users.Contact;
import com.example.novamailservice.entity.Users.ContactGroup;
import com.example.novamailservice.entity.Users.Users;
import com.example.novamailservice.mapper.Users.AddressBookMapper;
import com.example.novamailservice.service.User.AddressBookService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.regex.Pattern;

@Service
public class AddressBookServiceImpl implements AddressBookService {
    private static final Logger logger = LoggerFactory.getLogger(AddressBookServiceImpl.class);
    private static final Pattern EMAIL_PATTERN = Pattern.compile("^[^\\s@]+@[^\\s@]+\\.[^\\s@]+$");
    private static final int MAX_NAME_LENGTH = 255;
    private static final int MAX_EMAIL_LENGTH = 255;
    private static final int MAX_PHONE_LENGTH = 64;
    private static final int MAX_REMARK_LENGTH = 255;

    private final AddressBookMapper addressBookMapper;

    public AddressBookServiceImpl(AddressBookMapper addressBookMapper) {
        this.addressBookMapper = addressBookMapper;
    }

    @Override
    public ResponseResult<AddressBookData> getAddressBook(Users user) {
        if (!isValidUser(user)) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        try {
            return ResponseResult.success(new AddressBookData(
                    addressBookMapper.selectGroups(user.getUserId()),
                    addressBookMapper.selectContacts(user.getUserId())
            ));
        } catch (Exception e) {
            logger.error("通讯录获取失败", e);
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "通讯录获取失败");
        }
    }

    @Override
    @Transactional
    public ResponseResult<ContactGroup> saveGroup(Users user, SaveContactGroupRequest request) {
        if (!isValidUser(user)) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (request == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        String groupName = limitString(request.getGroupName(), MAX_NAME_LENGTH);
        if (groupName.isBlank()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "分组名称不能为空");
        if (addressBookMapper.countGroupName(user.getUserId(), groupName, request.getGroupId()) > 0) {
            return ResponseResult.error(ErrorCode.CONFLICT, "该分组名称已存在");
        }

        try {
            if (request.getGroupId() == null || request.getGroupId() <= 0) {
                ContactGroup group = new ContactGroup();
                group.setUserId(user.getUserId());
                group.setGroupName(groupName);
                group.setGroupType("CUSTOM");
                group.setSortOrder(addressBookMapper.selectMaxSortOrder(user.getUserId()) + 1);
                if (addressBookMapper.insertGroup(group) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "分组创建失败");
                return ResponseResult.success("保存成功", addressBookMapper.selectGroupById(user.getUserId(), group.getGroupId()));
            }

            ContactGroup current = addressBookMapper.selectGroupById(user.getUserId(), request.getGroupId());
            if (current == null) return ResponseResult.error(ErrorCode.NOT_FOUND, "分组不存在");
            if (!"CUSTOM".equals(current.getGroupType())) return ResponseResult.error(ErrorCode.FORBIDDEN, "系统分组不可编辑");
            if (addressBookMapper.updateGroupName(user.getUserId(), request.getGroupId(), groupName) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "分组更新失败");
            return ResponseResult.success("保存成功", addressBookMapper.selectGroupById(user.getUserId(), request.getGroupId()));
        } catch (Exception e) {
            logger.error("分组保存失败", e);
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "分组保存失败");
        }
    }

    @Override
    @Transactional
    public ResponseResult<String> deleteGroup(Users user, Long groupId) {
        if (!isValidUser(user)) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (groupId == null || groupId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "分组ID无效");

        ContactGroup current = addressBookMapper.selectGroupById(user.getUserId(), groupId);
        if (current == null) return ResponseResult.error(ErrorCode.NOT_FOUND, "分组不存在");
        if (!"CUSTOM".equals(current.getGroupType())) return ResponseResult.error(ErrorCode.FORBIDDEN, "系统分组不可删除");

        try {
            if (addressBookMapper.deleteCustomGroup(user.getUserId(), groupId) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "分组删除失败");
            return ResponseResult.success("删除成功");
        } catch (Exception e) {
            logger.error("分组删除失败", e);
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "分组删除失败");
        }
    }

    @Override
    @Transactional
    public ResponseResult<Contact> saveContact(Users user, SaveContactRequest request) {
        if (!isValidUser(user)) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (request == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        String contactName = limitString(request.getContactName(), MAX_NAME_LENGTH);
        String email = normalizeEmail(request.getEmail());
        if (contactName.isBlank() || email.isBlank()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "联系人姓名和邮箱不能为空");
        if (!EMAIL_PATTERN.matcher(email).matches()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱地址格式不正确");
        if (email.length() > MAX_EMAIL_LENGTH) return ResponseResult.error(ErrorCode.PARAM_INVALID, "邮箱地址过长");

        Long contactId = request.getContactId();
        if (addressBookMapper.countEmail(user.getUserId(), email, contactId) > 0) return ResponseResult.error(ErrorCode.CONFLICT, "该邮箱已经存在于通讯录中");

        Long groupId = request.getGroupId();
        if (groupId != null && groupId > 0 && addressBookMapper.selectGroupById(user.getUserId(), groupId) == null) {
            return ResponseResult.error(ErrorCode.NOT_FOUND, "分组不存在");
        }
        if (groupId != null && groupId <= 0) groupId = null;

        try {
            Contact contact = new Contact();
            contact.setContactId(contactId);
            contact.setUserId(user.getUserId());
            contact.setGroupId(groupId);
            contact.setContactName(contactName);
            contact.setEmail(email);
            contact.setPhone(limitNullableString(request.getPhone(), MAX_PHONE_LENGTH));
            contact.setRemark(limitNullableString(request.getRemark(), MAX_REMARK_LENGTH));

            if (contactId == null || contactId <= 0) {
                if (addressBookMapper.insertContact(contact) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "联系人创建失败");
                return ResponseResult.success("保存成功", addressBookMapper.selectContactById(user.getUserId(), contact.getContactId()));
            }

            if (addressBookMapper.selectContactById(user.getUserId(), contactId) == null) return ResponseResult.error(ErrorCode.NOT_FOUND, "联系人不存在");
            if (addressBookMapper.updateContact(contact) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "联系人更新失败");
            return ResponseResult.success("保存成功", addressBookMapper.selectContactById(user.getUserId(), contactId));
        } catch (Exception e) {
            logger.error("联系人保存失败", e);
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "联系人保存失败");
        }
    }

    @Override
    public ResponseResult<String> deleteContact(Users user, Long contactId) {
        if (!isValidUser(user)) return ResponseResult.error(ErrorCode.UNAUTHORIZED, "登录异常");
        if (contactId == null || contactId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "联系人ID无效");
        if (addressBookMapper.selectContactById(user.getUserId(), contactId) == null) return ResponseResult.error(ErrorCode.NOT_FOUND, "联系人不存在");

        try {
            if (addressBookMapper.deleteContact(user.getUserId(), contactId) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "联系人删除失败");
            return ResponseResult.success("删除成功");
        } catch (Exception e) {
            logger.error("联系人删除失败", e);
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "联系人删除失败");
        }
    }

    private boolean isValidUser(Users user) {
        return user != null && user.getUserId() != null;
    }

    private String normalizeEmail(String email) {
        if (email == null) return "";
        return email.trim().toLowerCase();
    }

    private String limitString(String value, int maxLength) {
        if (value == null) return "";
        String trimmed = value.trim();
        if (trimmed.codePointCount(0, trimmed.length()) <= maxLength) return trimmed;
        return trimmed.substring(0, trimmed.offsetByCodePoints(0, maxLength));
    }

    private String limitNullableString(String value, int maxLength) {
        String result = limitString(value, maxLength);
        return result.isBlank() ? null : result;
    }
}
