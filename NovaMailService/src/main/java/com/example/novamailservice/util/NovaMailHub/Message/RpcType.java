package com.example.novamailservice.util.NovaMailHub.Message;

public enum RpcType {
    UNKNOWN(0),
    REQUEST(1),
    RESPONSE(2);

    private final int value;

    RpcType(int value) {
        this.value = value;
    }

    public int value() {
        return value;
    }

    public static RpcType fromUInt8(int value) {
        return switch (value) {
            case 1 -> REQUEST;
            case 2 -> RESPONSE;
            default -> UNKNOWN;
        };
    }
}
