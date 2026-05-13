#include <core/Utils.hpp>
#include <cctype>

namespace NovaMail {
    namespace Smtp::Core {
        bool Utils::IEquals(std::string_view a, std::string_view b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) {
                unsigned char ca = static_cast<unsigned char>(a[i]);
                unsigned char cb = static_cast<unsigned char>(b[i]);
                if (std::tolower(ca) != std::tolower(cb)) return false;
            }
            return true;
        }
        
        bool Utils::IsBlank(const char c) {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';;
        }
    }
}
