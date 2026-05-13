package com.example.novamailservice.util.NovaMailHub.Message;

import lombok.Data;

@Data
public class RpcHeader {
    public static final int MAGIC = 0x4C48424D; // LHMB
    public static final int HEADER_SIZE = 40;
    public static final int MAX_METADATA_SIZE = 16 * 1024 * 1024;

    public static final int OFFSET_REQUEST_ID = 0;
    public static final int OFFSET_BODY_LENGTH = 8;
    public static final int OFFSET_MAGIC = 16;
    public static final int OFFSET_TIMEOUT_MS = 20;
    public static final int OFFSET_META_LENGTH = 24;
    public static final int OFFSET_VERSION = 28;
    public static final int OFFSET_REQUEST_TYPE = 30;
    public static final int OFFSET_FLAGS = 31;
    public static final int OFFSET_CODE = 32;
    public static final int OFFSET_RESERVED = 34;
    public static final int OFFSET_EXT_FLAGS = 36;

    private long requestId;
    private long bodyLength;
    private int magic = MAGIC;
    private int timeoutMs;
    private int metaLength;
    private int version = 1;
    private int requestType;
    private int flags;
    private int code;
    private int reserved;
    private long extFlags;

    public void setRequestType(int requestType) {
        this.requestType = requestType & 0xFF;
    }

    public void setFlags(int flags) {
        this.flags = flags & 0xFF;
    }

    public void setCode(int code) {
        this.code = code & 0xFFFF;
    }

    public void setReserved(int reserved) {
        this.reserved = reserved & 0xFFFF;
    }

    public void setExtFlags(long extFlags) {
        this.extFlags = extFlags & 0xFFFF_FFFFL;
    }

    public RpcType type() {
        return RpcType.fromUInt8(requestType);
    }

    public RpcCode codeEnum() {
        return RpcCode.fromUInt16(code);
    }
}
