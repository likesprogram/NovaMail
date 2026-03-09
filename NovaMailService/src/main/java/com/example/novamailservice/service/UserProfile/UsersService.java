package com.example.novamailservice.service.UserProfile;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Users.Users;

public interface UsersService {
    boolean createUser(Users user);
    ResponseResult<Users> getUserByAccount(String account);
    ResponseResult<Users> getUserById(Integer userId);
}
