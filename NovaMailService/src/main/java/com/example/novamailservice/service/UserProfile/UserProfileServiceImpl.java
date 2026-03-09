package com.example.novamailservice.service.UserProfile;

import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.Users.UserProfile;
import com.example.novamailservice.mapper.Users.UserProfileMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Service
public class UserProfileServiceImpl implements UserProfileService {
    private final UserProfileMapper userProfileMapper;
    @Autowired
    public UserProfileServiceImpl(UserProfileMapper userProfileMapper) {
        this.userProfileMapper = userProfileMapper;
    }

    @Override
    public boolean createUserProfile(UserProfile userProfile) {
        return userProfileMapper.createUserProfile(userProfile) > 0;
    }

    @Override
    public ResponseResult<UserProfile> getUserProfile(Long userId) {
        return null;
    }
}
