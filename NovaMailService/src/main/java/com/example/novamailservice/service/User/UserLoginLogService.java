package com.example.novamailservice.service.User;

import com.example.novamailservice.entity.Users.UserLoginLog;

import java.util.List;

public interface UserLoginLogService {
    boolean insert(UserLoginLog log);
    List<UserLoginLog> select();
    List<UserLoginLog> select(String account, Long maxLine);
    boolean delete(Long logId);
}
