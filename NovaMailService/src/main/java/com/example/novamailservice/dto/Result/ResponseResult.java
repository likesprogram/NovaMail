package com.example.novamailservice.dto.Result;


import lombok.Data;

@Data
public class ResponseResult<T> {
    private Integer code;
    private String message;
    private T data;

    private ResponseResult() {}

    private ResponseResult(Integer code, String message) {
        this.code = code;
        this.message = message;
    }

    private ResponseResult(Integer code, String message, T data) {
        this.code = code;
        this.message = message;
        this.data = data;
    }

    // 判断是否成功 内部使用
    public boolean isSuccess() {
        return this.code != null && this.code == ErrorCode.SUCCESS;
    }

    public static <T> ResponseResult<T> success() {
        return new ResponseResult<>(ErrorCode.SUCCESS, "操作成功");
    }

    public static <T> ResponseResult<T> success(T data) {
        return new ResponseResult<>(ErrorCode.SUCCESS, "操作成功", data);
    }

    public static <T> ResponseResult<T> success(String message, T data) {
        return new ResponseResult<>(ErrorCode.SUCCESS, message, data);
    }

    public static <T> ResponseResult<T> error() {
        return new ResponseResult<>(ErrorCode.INTERNAL_ERROR, "操作失败");
    }

    public static <T> ResponseResult<T> error(String message) {
        return new ResponseResult<>(ErrorCode.INTERNAL_ERROR, message);
    }

    public static <T> ResponseResult<T> error(Integer code, String message) {
        return new ResponseResult<>(code, message);
    }
}