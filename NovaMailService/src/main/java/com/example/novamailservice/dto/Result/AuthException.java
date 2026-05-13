package com.example.novamailservice.dto.Result;

public class AuthException extends RuntimeException {
    private final Integer code;

    public AuthException(Integer code, String message) {
        super(message);
        this.code = code;
    }

    public Integer getCode() {
        return code;
    }
}
