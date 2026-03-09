package com.example.novamailservice.dto.Result;

import lombok.Getter;

public class ServiceException extends RuntimeException {
    @Getter
    private final int code;
    private final String message;

    public ServiceException(String message) {
        this(ErrorCode.INTERNAL_ERROR, message); // 调用另一个构造器初始化当前对象
    }

    public ServiceException(Integer code, String message) {
        super(message);
        this.code = code;
        this.message = message;
    }

    @Override
    public String getMessage() {
        return message;
    }
}
