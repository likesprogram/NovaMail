#pragma once
#include <cstdint>
#include <cctype>
#include <string_view>

namespace NovaMail {
    namespace Smtp::Types {
        // SMTP 会话支持能力位掩码（用于 EHLO 能力协商与策略判断）
        enum class Capabilities : uint32_t {
            None            = 0, // 不支持任何扩展能力（纯 HELO / 最小模式）

            // 基础能力（决定是否进入 ESMTP 模式）
            ESMTP           = 1 << 0, // 支持 EHLO：允许客户端获取服务器能力列表
            PIPELINING      = 1 << 1, // 允许客户端连续发送多条命令而不等待响应（减少 RTT）
            SIZE            = 1 << 2, // 支持在 MAIL FROM 阶段声明邮件大小，用于提前拒绝超大邮件
            ENHANCED_STATUS = 1 << 3, // 返回增强状态码（x.y.z），便于客户端/日志精确判断失败原因
            DSN             = 1 << 4, // 支持投递状态通知（成功/失败/延迟的回执控制）

            // 连接安全与身份认证
            STARTTLS        = 1 << 5, // 支持在明文连接上升级为 TLS（加密 SMTP 会话）
            AUTH            = 1 << 6, // 支持 SMTP AUTH 命令（允许客户端进行身份认证）
            AUTH_PLAIN      = 1 << 7, // 支持 AUTH PLAIN 认证方式（简单、常用于提交端口）
            AUTH_LOGIN      = 1 << 8, // 支持 AUTH LOGIN 认证方式（兼容老客户端）

            // 邮件内容传输能力
            EIGHT_BIT_MIME  = 1 << 9, // 允许传输 8bit 内容，避免无意义的 quoted-printable/base64
            CHUNKING        = 1 << 10, // BDAT chunked transfer encoding, 允许分块传输邮件内容（跳过 DATA）
            BINARY_MIME     = 1 << 11, // 允许使用 CHUNKING 进行二进制块传输
            SMTP_UTF8       = 1 << 12, // 支持 UTF-8 邮箱地址与头部（国际化邮箱）

            // 地址验证与辅助命令
            VRFY            = 1 << 13, // 允许客户端验证邮箱地址是否存在（可能泄露用户信息）
            EXPN            = 1 << 14, // 允许展开邮件列表（极易被滥用，基本禁用）
            HELP            = 1 << 15, // 支持 HELP 命令，返回服务器支持的指令说明
        };

        Capabilities operator|(Capabilities a, Capabilities b);
        Capabilities& operator|=(Capabilities& a, Capabilities b);

        Capabilities operator&(Capabilities a, Capabilities b);

        bool HasCapability(Capabilities caps, Capabilities test);
    }
}
