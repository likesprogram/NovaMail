package com.example.novamailservice.dto.Result;

import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

@RestControllerAdvice
public class GlobalExceptionHandler {
    @ExceptionHandler(AuthException.class)
    public ResponseResult<Void> handleAuthException(AuthException e) {
        return ResponseResult.error(e.getCode(), e.getMessage());
    }

    @ExceptionHandler(Exception.class)
    public ResponseResult<Void> handleException(Exception e) {
        return ResponseResult.error("服务器内部错误");
    }
}
