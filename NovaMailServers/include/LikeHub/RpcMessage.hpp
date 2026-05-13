#pragma once
#include <LikesProgram/net/Buffer.hpp>
#include <LikesProgram/Configuration.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <arpa/inet.h>

namespace LikeHub {
    class RpcMessage {
    public:
        RpcMessage() = default;

        // 消息类型
        enum class Type : uint8_t {
            Unknown,        // 未知类型
            Request,        // 请求
            Response,       // 响应
            COUNT,          // 类型数量
        };
        static Type UInt8ToType(uint8_t v) noexcept {
            if (v < static_cast<uint8_t>(Type::COUNT)) return static_cast<Type>(v);
            return Type::Unknown;
        }
        static uint8_t TypeToUInt8(Type type) noexcept {
            return static_cast<uint8_t>(type);
        }
        static std::string TypeToString(Type type) noexcept {
            switch (type) {
                case Type::Request: return "Request";
                case Type::Response: return "Response";
                default: return "Unknown";
            }
        }

        // 消息代码
        enum class Code : uint16_t {
            None                = 0,    // 空
            // 通用
            OK                  = 200,  // 成功
            BadRequest          = 400,  // 请求参数错误 / metadata 格式错误 / 缺少必要字段
            Unauthorized        = 401,  // 认证失败 / 用户名或密码错误
            Forbidden           = 403,  // 无权限 / 白名单拒绝 / access 不允许
            NotFound            = 404,  // 目标不存在 / 事件无监听者 / 服务不存在
            Timeout             = 408,  // 请求超时
            Conflict            = 409,  // 重复注册 / 状态冲突
            PartialContent      = 410,  // 部分成功

            // Hub / 转发 / 服务调度
            DispatchFailed      = 502,  // 转发失败 / 目标处理异常
            ServiceUnavailable  = 503,  // 服务不可用 / 监听器不可用 / Hub 暂不可处理
            GatewayTimeout      = 504,  // 转发后等待下游响应超时

            // 内部错误
            InternalError       = 500   // Hub 内部异常 / 认证服务异常 / 未知异常
        };
        static uint16_t CodeToUInt16(Code code) noexcept {
            return static_cast<uint16_t>(code);
        }
        static Code UInt16ToCode(uint16_t code) noexcept {
            switch (code) {
                case 200: return Code::OK;
                case 400: return Code::BadRequest;
                case 401: return Code::Unauthorized;
                case 403: return Code::Forbidden;
                case 404: return Code::NotFound;
                case 408: return Code::Timeout;
                case 409: return Code::Conflict;
                case 410: return Code::PartialContent;
                case 500: return Code::InternalError;
                case 502: return Code::DispatchFailed;
                case 503: return Code::ServiceUnavailable;
                case 504: return Code::GatewayTimeout;
                default:  return Code::None;
            }
        }
        static std::string CodeToString(Code code) {
            switch (code) {
                case Code::OK:                 return "OK";
                case Code::BadRequest:         return "Bad Request";
                case Code::Unauthorized:       return "Unauthorized";
                case Code::Forbidden:          return "Forbidden";
                case Code::NotFound:           return "Not Found";
                case Code::Timeout:            return "Timeout";
                case Code::Conflict:           return "Conflict";
                case Code::PartialContent:     return "PartialContent";
                case Code::DispatchFailed:     return "Dispatch Failed";
                case Code::ServiceUnavailable: return "Service Unavailable";
                case Code::GatewayTimeout:     return "Gateway Timeout";
                case Code::InternalError:      return "Internal Error";
                default:                       return "Unknown";
            }
        }

        // 指令元数据，使用 Configuration 存储键值对
        using Metadata = LikesProgram::Configuration;
        using Bodydata = LikesProgram::Net::Buffer;

        #pragma pack(push, 8)
        struct RpcHeader {
            uint64_t requestId = 0;              // 请求ID
            uint64_t bodyLength = 0;             // 请求体长度
            // 8 + 8 = 16 字节

            uint32_t magic = MAGIC;              // 合法性魔数            
            uint32_t timeoutMs = 0;              // 请求超时时间（毫秒）
            // 4 * 2 = 8 字节 

            uint32_t metaLength = 0;             // 元数据长度
            uint16_t version = 1;                // 协议版本
            uint8_t requestType = 0;             // 消息类型
            uint8_t flags = 0;                   // 标志位，预留使用
            // 4 + 2 + 1 + 1 = 8 字节

            uint16_t code = 0;                   // 响应状态码，Request 可为 0
            uint16_t reserved = 0;               // 保留字段
            uint32_t extFlags = 0;               // 扩展标志位
            // 2 + 2 + 4 = 8 字节
            // 16 + 8 + 8 + 8 总计 40 字节, 设计为 8 字节对齐，方便网络传输和解析
        };
        #pragma pack(pop)

        // 解析返回结果
        enum class ParseStatus: uint8_t {
            Success,            // 解析成功
            Incomplete,         // 数据不完整，需要更多数据
            InvalidMagic,       // 魔数校验失败，非法数据包
            MetaLengthTooLarge, // 元数据长度超过限制
            BodyLengthTooLarge, // 请求体长度超过限制
            ParseError          // 其他解析错误
        };
        struct ParseResult {
            ParseStatus status;                  // 解析状态
            std::unique_ptr<RpcMessage> message; // 解析成功时的指令对象
            std::string errorMsg;                // 解析失败时的错误信息
        };

        ~RpcMessage() = default;
    public:
        // 尝试解析指令
        static ParseResult TryParse(LikesProgram::Net::Buffer& in, uint64_t maxBodySize = 0);

        // 编码指令
        static RpcMessage Encode(Type type, uint64_t requestId, Code code, const Metadata& metadata, const Bodydata& bodyData, uint32_t timeoutMs, uint16_t version = 1);
        static RpcMessage Encode(Type type, uint64_t requestId, Code code, const Metadata& metadata, uint32_t timeoutMs, uint16_t version = 1);
        static RpcMessage Encode(Type type, uint64_t requestId, Code code, uint32_t timeoutMs, uint16_t version = 1);

        // 指令头部
        const RpcHeader& Header() const noexcept;

        // 元数据
        const Metadata& MetaData() const noexcept;

        // 请求体数据
        const Bodydata& BodyData() const noexcept;

        // 指令转换 Buffer
        LikesProgram::Net::Buffer ToBuffer(uint64_t requestId) const;
    private:
        static constexpr uint32_t MAGIC = 0x4C48424D;                   // 'LHMB' in ASCII, 代表 LikeHub Message
        static constexpr size_t MAX_METADATA_SIZE = 16 * 1024 * 1024;   // 最大元数据大小（16MB）
        static constexpr size_t HEADER_SIZE = 40;                       // 指令头部固定大小
        static_assert(sizeof(RpcHeader) == HEADER_SIZE, "RpcHeader size must equal HEADER_SIZE");
        static_assert(offsetof(RpcHeader, requestId) == 0, "requestId offset mismatch");
        static_assert(offsetof(RpcHeader, bodyLength) == 8, "bodyLength offset mismatch");
        static_assert(offsetof(RpcHeader, magic) == 16, "magic offset mismatch");
        static_assert(offsetof(RpcHeader, timeoutMs) == 20, "timeoutMs offset mismatch");
        static_assert(offsetof(RpcHeader, metaLength) == 24, "metaLength offset mismatch");
        static_assert(offsetof(RpcHeader, version) == 28, "version offset mismatch");
        static_assert(offsetof(RpcHeader, requestType) == 30, "requestType offset mismatch");
        static_assert(offsetof(RpcHeader, flags) == 31, "flags offset mismatch");
        static_assert(offsetof(RpcHeader, code) == 32, "code offset mismatch");
        static_assert(offsetof(RpcHeader, reserved) == 34, "reserved offset mismatch");
        static_assert(offsetof(RpcHeader, extFlags) == 36, "extFlags offset mismatch");

        RpcHeader m_header;                         // 指令头部
        Metadata m_metadata;                        // 指令元数据
        Bodydata m_bodyData;                        // 指令体数据
    };
}
