package com.example.novamailservice.service.User.Impl;

import com.example.novamailservice.service.User.UserFilesService;
import org.springframework.stereotype.Service;

@Service
public class UserFilesServiceImpl implements UserFilesService {
    private static final String storageBucketName = "users";

    @Override
    public String getStorageBucketName() {
        return storageBucketName;
    }

    @Override
    public String getAvatarPath(String account) {
        return account + "/avatars";
    }
}
