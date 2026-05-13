package com.example.novamailservice.service.User.Impl;

import com.example.novamailservice.entity.Users.UserLoginLog;
import com.example.novamailservice.mapper.Users.UserLoginLogMapper;
import com.example.novamailservice.service.User.UserLoginLogService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class UserLoginLogServiceImpl implements UserLoginLogService {
    private final Logger logger = LoggerFactory.getLogger(UserLoginLogServiceImpl.class.getName());
    private final UserLoginLogMapper userLoginLogMapper;

    @Autowired
    public UserLoginLogServiceImpl(UserLoginLogMapper userLoginLogMapper) {
        this.userLoginLogMapper = userLoginLogMapper;
    }

    @Override
    public boolean insert(UserLoginLog log) {
        try {
            return userLoginLogMapper.insertLoginLog(log) > 0;
        } catch (Exception e) {
            logger.warn("UserLoginLog 插入失败", e);

            Throwable root = e;
            while (root.getCause() != null) root = root.getCause();
            logger.error("Root cause: {}", root.getMessage(), root);

            return false;
        }
    }

    @Override
    public List<UserLoginLog> select() {
        return userLoginLogMapper.selectLoginLog();
    }

    @Override
    public List<UserLoginLog> select(String account, Long maxLine) {
        return userLoginLogMapper.selectLoginLogByUserId(account, maxLine);
    }

    @Override
    public boolean delete(Long logId) {
        return userLoginLogMapper.deleteLoginLog(logId) > 0;
    }
}
