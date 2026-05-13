package com.example.novamailservice.service.User;

import jakarta.servlet.http.HttpServletResponse;

public interface UserAvatarService {
    void writeAvatarByAccount(String account, HttpServletResponse response) throws Exception;
    void writeDefaultAvatar(HttpServletResponse response) throws Exception;
}
