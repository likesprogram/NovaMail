#include <types/Types.hpp>
#include <types/MailBody.hpp>
#include <core/Utils.hpp>

namespace NovaMail {
    namespace Smtp::Types {
        MailBody ParseMailBody(std::string_view value) {
            if (Core::Utils::IEquals(value, "7BIT")) return MailBody::Bit7;
            if (Core::Utils::IEquals(value, "8BITMIME")) return MailBody::Bit8Mime;
            if (Core::Utils::IEquals(value, "BINARYMIME")) return MailBody::BinaryMime;
            return MailBody::Unspecified;
        }

        const std::string MailBodyToString(MailBody body) {
            switch (body) {
            case MailBody::Bit7: return "7BIT";
            case MailBody::Bit8Mime: return "8BITMIME";
            case MailBody::BinaryMime: return "BINARYMIME";
            default: return "UNSPECIFIED";
            }
        }
    }
}
