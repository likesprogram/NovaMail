#include <types/RcptNotify.hpp>

namespace NovaMail {
    namespace Smtp::Types {   
        RcptNotify operator|(RcptNotify a, RcptNotify b) {
            return static_cast<RcptNotify>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
        }

        RcptNotify operator&(RcptNotify a, RcptNotify b) {
            return static_cast<RcptNotify>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
        }

        bool HasNotify(RcptNotify flags, RcptNotify test) {
            return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(test)) != 0;
        }

        bool IsValidNotify(RcptNotify flags) {
            if (HasNotify(flags, RcptNotify::Never) && flags != RcptNotify::Never) return false;
            return true;
        }
    }
}