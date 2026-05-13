package com.example.novamailservice.util.NovaMailHub.Message;

public enum RpcCode {
    NONE(0),

    OK(200),
    BAD_REQUEST(400),
    UNAUTHORIZED(401),
    FORBIDDEN(403),
    NOT_FOUND(404),
    TIMEOUT(408),
    CONFLICT(409),

    INTERNAL_ERROR(500),
    DISPATCH_FAILED(502),
    SERVICE_UNAVAILABLE(503),
    GATEWAY_TIMEOUT(504);

    private final int value;

    RpcCode(int value) {
        this.value = value;
    }

    public int value() {
        return value;
    }

    public static RpcCode fromUInt16(int value) {
        return switch (value) {
            case 200 -> OK;
            case 400 -> BAD_REQUEST;
            case 401 -> UNAUTHORIZED;
            case 403 -> FORBIDDEN;
            case 404 -> NOT_FOUND;
            case 408 -> TIMEOUT;
            case 409 -> CONFLICT;
            case 500 -> INTERNAL_ERROR;
            case 502 -> DISPATCH_FAILED;
            case 503 -> SERVICE_UNAVAILABLE;
            case 504 -> GATEWAY_TIMEOUT;
            default -> NONE;
        };
    }

    public String text() {
        return switch (this) {
            case OK -> "OK";
            case BAD_REQUEST -> "Bad Request";
            case UNAUTHORIZED -> "Unauthorized";
            case FORBIDDEN -> "Forbidden";
            case NOT_FOUND -> "Not Found";
            case TIMEOUT -> "Timeout";
            case CONFLICT -> "Conflict";
            case INTERNAL_ERROR -> "Internal Error";
            case DISPATCH_FAILED -> "Dispatch Failed";
            case SERVICE_UNAVAILABLE -> "Service Unavailable";
            case GATEWAY_TIMEOUT -> "Gateway Timeout";
            default -> "Unknown";
        };
    }
}