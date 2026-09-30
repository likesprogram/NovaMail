package com.example.novamailservice.service.User;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.Users.AddressBookData;
import com.example.novamailservice.dto.Users.SaveContactGroupRequest;
import com.example.novamailservice.dto.Users.SaveContactRequest;
import com.example.novamailservice.entity.Users.Contact;
import com.example.novamailservice.entity.Users.ContactGroup;
import com.example.novamailservice.entity.Users.Users;

public interface AddressBookService {
    ResponseResult<AddressBookData> getAddressBook(Users user);
    ResponseResult<ContactGroup> saveGroup(Users user, SaveContactGroupRequest request);
    ResponseResult<String> deleteGroup(Users user, Long groupId);
    ResponseResult<Contact> saveContact(Users user, SaveContactRequest request);
    ResponseResult<String> deleteContact(Users user, Long contactId);
}
