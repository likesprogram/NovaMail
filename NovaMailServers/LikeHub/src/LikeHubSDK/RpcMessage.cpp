#include <LikeHub/RpcMessage.hpp>
#include <LikeHub/NetEndian.hpp>
#include <cassert>
#include <limits>

namespace LikeHub {
    RpcMessage::ParseResult RpcMessage::TryParse(LikesProgram::Net::Buffer& in, uint64_t maxBodySize) {
        // Header 不完整，直接返回（典型 TCP 粘包/半包场景）
        if (in.ReadableBytes() < HEADER_SIZE) return ParseResult { ParseStatus::Incomplete, nullptr, "Incomplete data" };
        const uint8_t* p = in.Peek();
        // 固定 Header 布局：
        // 0  - 7   requestId
        // 8  - 15  bodyLength
        // 16 - 19  magic
        // 20 - 23  timeoutMs
        // 24 - 27  metaLength
        // 28 - 29  version
        // 30       requestType
        // 31       flags
        // 32 - 33  code
        // 34 - 35  reserved
        // 36 - 39  extFlags

        // 解析必要 Header 字段
        uint64_t requestId  = NetEndian::ReadNet<uint64_t>(p + 0);
        uint64_t bodyLength = NetEndian::ReadNet<uint64_t>(p + 8);
        uint32_t magic      = NetEndian::ReadNet<uint32_t>(p + 16);
        uint32_t timeoutMs  = NetEndian::ReadNet<uint32_t>(p + 20);
        uint32_t metaLength = NetEndian::ReadNet<uint32_t>(p + 24);

        // 安全校验：魔数校验（防止垃圾数据攻击）
        if (magic != MAGIC) return ParseResult { ParseStatus::InvalidMagic, nullptr, "Invalid magic number" };

        // 安全校验：元数据长度超过限制（防止内存攻击 / DoS）
        if (metaLength > MAX_METADATA_SIZE) return ParseResult{ ParseStatus::MetaLengthTooLarge, nullptr, "Metadata length too large" };
        
        // 安全校验：bodyLength 长度限制（防止内存攻击 / DoS）
        if (bodyLength > maxBodySize) return ParseResult { ParseStatus::BodyLengthTooLarge, nullptr, "Body length too large" };
        // 长度限制
        const uint64_t totalLength = static_cast<uint64_t>(HEADER_SIZE) + static_cast<uint64_t>(metaLength) + bodyLength;
        if (totalLength > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
            return ParseResult { ParseStatus::BodyLengthTooLarge, nullptr, "Rpc message too large for this platform" };
        }

        // 包不完整（meta + body 还没收全）
        if (in.ReadableBytes() < static_cast<size_t>(totalLength)) return ParseResult { ParseStatus::Incomplete, nullptr, "Incomplete data" };

        // 解析其他 Header 字段
        uint16_t version    = NetEndian::ReadNet<uint16_t>(p + 28);
        uint8_t requestType = NetEndian::ReadNet<uint8_t>(p + 30);
        uint8_t flags       = NetEndian::ReadNet<uint8_t>(p + 31);
        uint16_t code       = NetEndian::ReadNet<uint16_t>(p + 32);
        uint16_t reserved   = NetEndian::ReadNet<uint16_t>(p + 34);
        uint32_t extFlags   = NetEndian::ReadNet<uint32_t>(p + 36);

        // 创建返回消息
        std::unique_ptr<RpcMessage> out(new RpcMessage());
        // 填充 Header
        out->m_header.requestId = requestId;
        out->m_header.bodyLength = bodyLength;
        out->m_header.magic = magic;
        out->m_header.timeoutMs = timeoutMs;
        out->m_header.metaLength = metaLength;
        out->m_header.version = version;
        out->m_header.requestType = requestType;
        out->m_header.flags = flags;
        out->m_header.code = code;
        out->m_header.reserved = reserved;
        out->m_header.extFlags = extFlags;

        try {
            const uint8_t* metaPtr = p + HEADER_SIZE;
            const uint8_t* bodyPtr = metaPtr + metaLength;

            // 解析元数据（JSON）
            if (metaLength > 0) {
                std::string metaString(reinterpret_cast<const char*>(metaPtr), metaLength);
                out->m_metadata.Load(LikesProgram::String(metaString)); // 反序列化元数据
            }

            // 消解析请求体数据
            if (bodyLength > 0) out->m_bodyData.Append(bodyPtr, static_cast<size_t>(bodyLength));

            // 解析成功，消耗掉这个包
            in.Consume(static_cast<size_t>(totalLength));
        } catch (const std::exception& ex) {
            // 消费掉这个包，防止粘包攻击
            in.Consume(static_cast<size_t>(totalLength));
            return ParseResult { ParseStatus::ParseError, nullptr, std::string("Parse error: ") + ex.what() };
        }

        return ParseResult { ParseStatus::Success, std::move(out), "" };
    }

    RpcMessage RpcMessage::Encode(Type type, uint64_t requestId, Code code, const Metadata& metadata, const Bodydata& bodyData, uint32_t timeoutMs, uint16_t version) {
        // 序列化元数据
        std::string metaString = metadata.Dump(0).ToStdString(); // 紧凑格式序列化元数据
        size_t metaLength = metaString.size();
        assert(metaLength <= MAX_METADATA_SIZE);

        RpcMessage out;

        // 写入 header
        out.m_header.requestId = requestId;
        out.m_header.bodyLength = bodyData.ReadableBytes();
        out.m_header.magic = MAGIC;
        out.m_header.timeoutMs = timeoutMs;
        out.m_header.metaLength = static_cast<uint32_t>(metaLength);
        out.m_header.version = version;
        out.m_header.requestType = RpcMessage::TypeToUInt8(type);
        out.m_header.flags = 0;
        out.m_header.code = RpcMessage::CodeToUInt16(code);
        out.m_header.reserved = 0;
        out.m_header.extFlags = 0;

        // 写入元数据和请求体数据
        out.m_metadata = metadata;
        out.m_bodyData = bodyData;
        return out;
    }

    RpcMessage RpcMessage::Encode(Type type, uint64_t requestId, Code code, const Metadata& metadata, uint32_t timeoutMs, uint16_t version) {
        // 序列化元数据
        std::string metaString = metadata.Dump(0).ToStdString(); // 紧凑格式序列化元数据
        size_t metaLength = metaString.size();
        assert(metaLength <= MAX_METADATA_SIZE);

        RpcMessage out;

        // 写入 header
        out.m_header.requestId = requestId;
        out.m_header.bodyLength = 0; // 没有请求体数据
        out.m_header.magic = MAGIC;
        out.m_header.timeoutMs = timeoutMs;
        out.m_header.metaLength = static_cast<uint32_t>(metaLength);
        out.m_header.version = version;
        out.m_header.requestType = RpcMessage::TypeToUInt8(type);
        out.m_header.flags = 0;
        out.m_header.code = RpcMessage::CodeToUInt16(code);
        out.m_header.reserved = 0;
        out.m_header.extFlags = 0;

        // 写入元数据
        out.m_metadata = metadata;
        return out;
    }

    RpcMessage RpcMessage::Encode(Type type, uint64_t requestId, Code code, uint32_t timeoutMs, uint16_t version) {
        RpcMessage out;

        // 写入 header
        out.m_header.requestId = requestId;
        out.m_header.bodyLength = 0; // 没有请求体数据
        out.m_header.magic = MAGIC;
        out.m_header.timeoutMs = timeoutMs;
        out.m_header.metaLength = 0; // 没有元数据
        out.m_header.version = version;
        out.m_header.requestType = RpcMessage::TypeToUInt8(type);
        out.m_header.flags = 0;
        out.m_header.code = RpcMessage::CodeToUInt16(code);
        out.m_header.reserved = 0;
        out.m_header.extFlags = 0;

        return out;
    }

    const RpcMessage::RpcHeader& RpcMessage::Header() const noexcept {
        return m_header;
    }

    const RpcMessage::Metadata& RpcMessage::MetaData() const noexcept {
        return m_metadata;
    }

    const RpcMessage::Bodydata& RpcMessage::BodyData() const noexcept {
        return m_bodyData;
    }

    LikesProgram::Net::Buffer RpcMessage::ToBuffer(uint64_t requestId) const {
        std::string metaString;

        if (!m_metadata.IsNull()) metaString = m_metadata.Dump(0).ToStdString();
        const uint32_t actualMetaLength = static_cast<uint32_t>(metaString.size());
        const uint64_t actualBodyLength = m_bodyData.ReadableBytes();

        uint8_t header[HEADER_SIZE];
        NetEndian::WriteNet<uint64_t>(header + 0, m_header.requestId == 0 ? requestId : m_header.requestId);
        NetEndian::WriteNet<uint64_t>(header + 8, actualBodyLength);
        NetEndian::WriteNet<uint32_t>(header + 16, m_header.magic);
        NetEndian::WriteNet<uint32_t>(header + 20, m_header.timeoutMs);
        NetEndian::WriteNet<uint32_t>(header + 24, actualMetaLength);
        NetEndian::WriteNet<uint16_t>(header + 28, m_header.version);
        NetEndian::WriteNet<uint8_t>(header + 30, m_header.requestType);
        NetEndian::WriteNet<uint8_t>(header + 31, m_header.flags);
        NetEndian::WriteNet<uint16_t>(header + 32, m_header.code);
        NetEndian::WriteNet<uint16_t>(header + 34, m_header.reserved);
        NetEndian::WriteNet<uint32_t>(header + 36, m_header.extFlags);
        LikesProgram::Net::Buffer out;

        out.Append(header, HEADER_SIZE);
        if (actualMetaLength > 0) out.Append(metaString.data(), metaString.size());
        if (actualBodyLength > 0)  out.Append(m_bodyData.Peek(), static_cast<size_t>(actualBodyLength));

        return out;
    }
}
