#pragma once
#include <string_view>
#include <cstdint>
#include <span>

namespace NovaMail {
    namespace Smtp::Types {
        // 指令动词
        enum class CommandVerb: uint16_t {
            EHLO, HELO,
            MAIL, RCPT, DATA,
            RSET, NOOP, QUIT,
            VRFY, EXPN, HELP,
            SEND, SOML, SAML,
            STARTTLS, AUTH, BDAT,
            ETRN, ATRN, TURN,
            COUNT,   // 用于数组大小 (若后续添加指令，需要写在前面)
            Unknown
        };
        constexpr const size_t CommandVerbCount = static_cast<size_t>(CommandVerb::COUNT);
        CommandVerb StringToVerb(std::string_view verb) noexcept;
    }
}
