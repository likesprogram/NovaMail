#pragma once
#include <cstdint>
#include <cctype>
#include <string_view>

namespace NovaMail {
    namespace Smtp {
        // SMTP 指令状态
        enum class State : int {
            Connected = 0,       // TCP 已建立，未发 220
            Greeted = 1,         // 已发 220
            Identified = 2,      // EHLO/HELO 完成（会话就绪/Idle）
            MailStarted = 3,     // MAIL FROM 成功（事务进行中）
            RcptCollecting = 4,  // 至少一个 RCPT
            DataReceiving = 5,   // DATA 内容接收中
            TlsNegotiating = 6,  // STARTTLS 已接受，TLS 握手中
            DataReceiptCompleted = 7, // 接收完成
        };

        // SMTP 会话支持能力位掩码（用于 EHLO 能力协商与策略判断）
        enum class Capabilities : int {
            None            = 0x00000000, // 不支持任何扩展能力（纯 HELO / 最小模式）

            // 基础能力（决定是否进入 ESMTP 模式）
            ESMTP           = 0x00000001, // 支持 EHLO：允许客户端获取服务器能力列表
            PIPELINING      = 0x00000002, // 允许客户端连续发送多条命令而不等待响应（减少 RTT）
            SIZE            = 0x00000004, // 支持在 MAIL FROM 阶段声明邮件大小，用于提前拒绝超大邮件
            ENHANCED_STATUS = 0x00000008, // 返回增强状态码（x.y.z），便于客户端/日志精确判断失败原因
            DSN             = 0x00000010, // 支持投递状态通知（成功/失败/延迟的回执控制）

            // 连接安全与身份认证
            STARTTLS        = 0x00000020, // 支持在明文连接上升级为 TLS（加密 SMTP 会话）
            AUTH            = 0x00000040, // 支持 SMTP AUTH 命令（允许客户端进行身份认证）
            AUTH_PLAIN      = 0x00000080, // 支持 AUTH PLAIN 认证方式（简单、常用于提交端口）
            AUTH_LOGIN      = 0x00000100, // 支持 AUTH LOGIN 认证方式（兼容老客户端）
            AUTH_CRAM_MD5   = 0x00000200, // 支持 CRAM-MD5 挑战响应认证（无明文密码，较少使用）

            // 邮件内容传输能力
            EIGHT_BIT_MIME  = 0x00000400, // 允许传输 8bit 内容，避免无意义的 quoted-printable/base64
            BINARY_MIME     = 0x00000800, // 允许使用 BDAT/CHUNKING 进行二进制块传输（跳过 DATA）
            SMTP_UTF8       = 0x00001000, // 支持 UTF-8 邮箱地址与头部（国际化邮箱）

            // 地址验证与辅助命令
            VRFY            = 0x00002000, // 允许客户端验证邮箱地址是否存在（可能泄露用户信息）
            EXPN            = 0x00004000, // 允许展开邮件列表（极易被滥用，基本禁用）
            HELP            = 0x00008000, // 支持 HELP 命令，返回服务器支持的指令说明
        };

        inline Capabilities operator|(Capabilities a, Capabilities b) {
            return static_cast<Capabilities>(
                static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
            );
        }

        inline Capabilities operator&(Capabilities a, Capabilities b) {
            return static_cast<Capabilities>(
                static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
            );
        }

        // 忽略大小写比较字符串
        inline bool IEquals(std::string_view a, std::string_view b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) {
                unsigned char ca = static_cast<unsigned char>(a[i]);
                unsigned char cb = static_cast<unsigned char>(b[i]);
                if (std::tolower(ca) != std::tolower(cb)) return false;
            }
            return true;
        }

        // 指令动词
        enum class CommandVerb {
            EHLO, HELO,
            MAIL, RCPT, DATA,
            RSET, NOOP, QUIT,
            VRFY, EXPN, HELP,
            SEND, SOML, SAML,
            STARTTLS, AUTH, BDAT,
            ETRN, ATRN, TURN,
            Unknown
        };
    }
}
