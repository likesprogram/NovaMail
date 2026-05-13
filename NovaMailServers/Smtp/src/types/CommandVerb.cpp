#include <types/CommandVerb.hpp>
#include <types/Types.hpp>
#include <core/Utils.hpp>

namespace NovaMail {
    namespace Smtp::Types {
        CommandVerb StringToVerb(std::string_view verb) noexcept {
            // 基础命令
            if (Core::Utils::IEquals(verb, "EHLO")) return CommandVerb::EHLO;
            if (Core::Utils::IEquals(verb, "HELO")) return CommandVerb::HELO;
        
            // 事务命令
            if (Core::Utils::IEquals(verb, "MAIL")) return CommandVerb::MAIL;  // MAIL FROM:...
            if (Core::Utils::IEquals(verb, "RCPT")) return CommandVerb::RCPT;  // RCPT TO:...
            if (Core::Utils::IEquals(verb, "DATA")) return CommandVerb::DATA;  // DATA + <CRLF>.<CRLF>
        
            if (Core::Utils::IEquals(verb, "RSET")) return CommandVerb::RSET;
            if (Core::Utils::IEquals(verb, "NOOP")) return CommandVerb::NOOP;
            if (Core::Utils::IEquals(verb, "QUIT")) return CommandVerb::QUIT;
        
            if (Core::Utils::IEquals(verb, "VRFY")) return CommandVerb::VRFY;  // 常见：通常禁用/返回252或502
            if (Core::Utils::IEquals(verb, "EXPN")) return CommandVerb::EXPN;  // 常见：通常禁用/返回502
            if (Core::Utils::IEquals(verb, "HELP")) return CommandVerb::HELP;
        
            // RFC 5321 遗留邮件发送族
            if (Core::Utils::IEquals(verb, "SEND")) return CommandVerb::SEND;
            if (Core::Utils::IEquals(verb, "SOML")) return CommandVerb::SOML;
            if (Core::Utils::IEquals(verb, "SAML")) return CommandVerb::SAML;
        
            // ESMTP 扩展
            if (Core::Utils::IEquals(verb, "STARTTLS")) return CommandVerb::STARTTLS; // STARTTLS
            if (Core::Utils::IEquals(verb, "AUTH")) return CommandVerb::AUTH;     // AUTH <mech> ...
            if (Core::Utils::IEquals(verb, "BDAT")) return CommandVerb::BDAT;     // CHUNKING：BDAT <size> [LAST]
            
            // 其它扩展/队列触发
            if (Core::Utils::IEquals(verb, "ETRN")) return CommandVerb::ETRN;     // 队列触发（on-demand mail relay）
            if (Core::Utils::IEquals(verb, "ATRN")) return CommandVerb::ATRN;     // Authenticated TURN
        
            // 历史遗留
            if (Core::Utils::IEquals(verb, "TURN")) return CommandVerb::TURN;     // 历史命令，通常直接502
        
            return CommandVerb::Unknown;
        }
    }
}
