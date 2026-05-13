#pragma once
#include <cstdint>
#include <string>
#include <string>

namespace NovaMail {
    namespace Smtp::Types {
        enum class MailBody : uint8_t {
            Unspecified = 0,
            Bit7,
            Bit8Mime,
            BinaryMime
        };
        
        MailBody ParseMailBody(std::string_view value);

        const std::string MailBodyToString(MailBody body);
    }
}
