#include <types/Capabilities.hpp>

namespace NovaMail {
    namespace Smtp::Types {
        Capabilities operator|(Capabilities a, Capabilities b) {
            return static_cast<Capabilities>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
        }

        Capabilities& operator|=(Capabilities& a, Capabilities b) {
            a = a | b;
            return a;
        }
        
        Capabilities operator&(Capabilities a, Capabilities b) {
            return static_cast<Capabilities>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
        }

        bool HasCapability(Capabilities caps, Capabilities test) {
            return (static_cast<uint32_t>(caps) & static_cast<uint32_t>(test)) != 0;
        }
    }
}
