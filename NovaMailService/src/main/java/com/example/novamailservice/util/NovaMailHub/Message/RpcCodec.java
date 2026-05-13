package com.example.novamailservice.util.NovaMailHub.Message;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import io.netty.buffer.ByteBuf;

import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

public final class RpcCodec {
    private static final ObjectMapper OBJECT_MAPPER = new ObjectMapper();

    private RpcCodec() {}

    public static RpcMessage encode(RpcType type, long requestId, RpcCode code, Map<String, Object> metadata, byte[] body, int timeoutMs, int version) {
        RpcMessage message = new RpcMessage();

        message.setRequestId(requestId);
        message.setType(type);
        message.setCode(code);
        message.setTimeoutMs(timeoutMs);
        message.setVersion(version);
        message.setMetadata(metadata);
        message.setBody(body);

        return message;
    }

    public static RpcMessage encode(RpcType type, long requestId, RpcCode code, Map<String, Object> metadata, int timeoutMs, int version) {
        return encode(type, requestId, code, metadata, new byte[0], timeoutMs, version);
    }

    public static RpcMessage encode(RpcType type, long requestId, RpcCode code, int timeoutMs, int version) {
        return encode(type, requestId, code, new HashMap<>(), new byte[0], timeoutMs, version);
    }

    public static void writeToByteBuf(RpcMessage message, ByteBuf out) {
        try {
            byte[] metadataBytes = metadataToBytes(message.getMetadata());
            byte[] body = message.getBody() == null ? new byte[0] : message.getBody();

            if (metadataBytes.length > RpcHeader.MAX_METADATA_SIZE) {
                throw new IllegalArgumentException("Metadata length too large: " + metadataBytes.length);
            }

            RpcHeader header = message.getHeader();

            header.setMagic(RpcHeader.MAGIC);
            header.setMetaLength(metadataBytes.length);
            header.setBodyLength(body.length);

            out.writeLong(header.getRequestId());
            out.writeLong(header.getBodyLength());
            out.writeInt(header.getMagic());
            out.writeInt(header.getTimeoutMs());
            out.writeInt(header.getMetaLength());
            out.writeShort(header.getVersion());
            out.writeByte(header.getRequestType());
            out.writeByte(header.getFlags());
            out.writeShort(header.getCode());
            out.writeShort(header.getReserved());
            out.writeInt((int) header.getExtFlags());

            if (metadataBytes.length > 0) {
                out.writeBytes(metadataBytes);
            }

            if (body.length > 0) {
                out.writeBytes(body);
            }
        } catch (Exception e) {
            throw new IllegalStateException("Encode RpcMessage failed", e);
        }
    }

    public static RpcParseResult tryParse(ByteBuf in, long maxBodySize) {
        int readableBytes = in.readableBytes();
        if (in.readableBytes() < RpcHeader.HEADER_SIZE) return RpcParseResult.fail(RpcParseResult.RpcParseStatus.INCOMPLETE, "Incomplete data");

        in.markReaderIndex();

        long requestId = in.readLong();
        long bodyLength = in.readLong();
        int magic = in.readInt();
        int timeoutMs = in.readInt();
        int metaLength = in.readInt();

        if (magic != RpcHeader.MAGIC) {
            in.resetReaderIndex();
            return RpcParseResult.fail(RpcParseResult.RpcParseStatus.INVALID_MAGIC, "Invalid magic number");
        }

        if (metaLength < 0 || metaLength > RpcHeader.MAX_METADATA_SIZE) {
            in.resetReaderIndex();
            return RpcParseResult.fail(RpcParseResult.RpcParseStatus.META_LENGTH_TOO_LARGE, "Metadata length too large");
        }

        if (bodyLength < 0 || bodyLength > maxBodySize) {
            in.resetReaderIndex();
            return RpcParseResult.fail(RpcParseResult.RpcParseStatus.BODY_LENGTH_TOO_LARGE, "Body length too large");
        }

        long packetSize = RpcHeader.HEADER_SIZE + (long) metaLength + bodyLength;

        if (packetSize > Integer.MAX_VALUE) {
            in.resetReaderIndex();
            return RpcParseResult.fail(RpcParseResult.RpcParseStatus.BODY_LENGTH_TOO_LARGE, "Packet length too large");
        }
        int alreadyReadHeaderSize = 8 + 8 + 4 + 4 + 4;
        int remainingHeaderSize = RpcHeader.HEADER_SIZE - alreadyReadHeaderSize;

        long remainingPacketSize = remainingHeaderSize + (long) metaLength + bodyLength;
        if (in.readableBytes() < remainingPacketSize) {
            in.resetReaderIndex();
            return RpcParseResult.fail(RpcParseResult.RpcParseStatus.INCOMPLETE, "Incomplete data");
        }

        int version = in.readUnsignedShort();
        int requestType = in.readUnsignedByte();
        int flags = in.readUnsignedByte();
        int code = in.readUnsignedShort();
        int reserved = in.readUnsignedShort();
        long extFlags = Integer.toUnsignedLong(in.readInt());

        try {
            byte[] metadataBytes = new byte[metaLength];
            if (metaLength > 0) in.readBytes(metadataBytes);

            byte[] bodyBytes = new byte[(int) bodyLength];
            if (bodyLength > 0) in.readBytes(bodyBytes);

            Map<String, Object> metadata = bytesToMetadata(metadataBytes);

            RpcMessage message = new RpcMessage();
            message.setRequestId(requestId);
            message.setTimeoutMs(timeoutMs);
            message.setVersion(version);
            message.setType(RpcType.fromUInt8(requestType));
            message.setFlags(flags);
            message.setCode(RpcCode.fromUInt16(code));
            message.getHeader().setMagic(magic);
            message.getHeader().setMetaLength(metaLength);
            message.getHeader().setBodyLength(bodyLength);
            message.getHeader().setReserved(reserved);
            message.getHeader().setExtFlags(extFlags);
            message.setMetadata(metadata);
            message.setBody(bodyBytes);

            return RpcParseResult.success(message);
        } catch (Exception e) {
            return RpcParseResult.fail(RpcParseResult.RpcParseStatus.PARSE_ERROR, "Parse error: " + e.getMessage());
        }
    }

    public static byte[] metadataToBytes(Map<String, Object> metadata) throws Exception {
        if (metadata == null || metadata.isEmpty()) return new byte[0];

        return OBJECT_MAPPER.writeValueAsString(metadata).getBytes(StandardCharsets.UTF_8);
    }

    public static Map<String, Object> bytesToMetadata(byte[] metadataBytes) throws Exception {
        if (metadataBytes == null || metadataBytes.length == 0) return new HashMap<>();

        String json = new String(metadataBytes, StandardCharsets.UTF_8);

        return OBJECT_MAPPER.readValue(json, new TypeReference<Map<String, Object>>() {});
    }
}