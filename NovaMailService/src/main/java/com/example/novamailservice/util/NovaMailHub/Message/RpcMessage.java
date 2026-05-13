package com.example.novamailservice.util.NovaMailHub.Message;

import lombok.Data;

import lombok.Data;

import java.util.HashMap;
import java.util.Map;

@Data
public class RpcMessage {
    private final RpcHeader header = new RpcHeader();

    private Map<String, Object> metadata = new HashMap<>();
    private byte[] body = new byte[0];

    public long getRequestId() {
        return header.getRequestId();
    }

    public void setRequestId(long requestId) {
        header.setRequestId(requestId);
    }

    public RpcType getType() {
        return header.type();
    }

    public void setType(RpcType type) {
        header.setRequestType(type == null ? RpcType.UNKNOWN.value() : type.value());
    }

    public RpcCode getCode() {
        return header.codeEnum();
    }

    public void setCode(RpcCode code) {
        header.setCode(code == null ? RpcCode.NONE.value() : code.value());
    }

    public int getTimeoutMs() {
        return header.getTimeoutMs();
    }

    public void setTimeoutMs(int timeoutMs) {
        header.setTimeoutMs(timeoutMs);
    }

    public int getVersion() {
        return header.getVersion();
    }

    public void setVersion(int version) {
        header.setVersion(version);
    }

    public int getFlags() {
        return header.getFlags();
    }

    public void setFlags(int flags) {
        header.setFlags(flags);
    }

    public long getExtFlags() {
        return header.getExtFlags();
    }

    public void setExtFlags(long extFlags) {
        header.setExtFlags(extFlags);
    }

    public void setMetadata(Map<String, Object> metadata) {
        this.metadata = metadata == null ? new HashMap<>() : new HashMap<>(metadata);
    }

    public void setBody(byte[] body) {
        this.body = body == null ? new byte[0] : body;
    }
}
