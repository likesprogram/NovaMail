package com.example.novamailservice.util.NovaMailHub.Message;

import lombok.Data;

@Data
public class RpcParseResult {
    public enum RpcParseStatus {
        SUCCESS,
        INCOMPLETE,
        INVALID_MAGIC,
        META_LENGTH_TOO_LARGE,
        BODY_LENGTH_TOO_LARGE,
        PARSE_ERROR
    }

    private final RpcParseStatus status;
    private final RpcMessage message;
    private final String errorMessage;

    private RpcParseResult(RpcParseStatus status, RpcMessage message, String errorMessage) {
        this.status = status;
        this.message = message;
        this.errorMessage = errorMessage;
    }

    public static RpcParseResult success(RpcMessage message) {
        return new RpcParseResult(RpcParseStatus.SUCCESS, message, "");
    }

    public static RpcParseResult fail(RpcParseStatus status, String errorMessage) {
        return new RpcParseResult(status, null, errorMessage);
    }

    public boolean isSuccess() {
        return status == RpcParseStatus.SUCCESS;
    }

    public boolean isIncomplete() {
        return status == RpcParseStatus.INCOMPLETE;
    }
}
