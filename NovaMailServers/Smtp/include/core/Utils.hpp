#pragma once
#include <string_view>

namespace NovaMail {
    namespace Smtp::Core {
        struct Utils {
            // 忽略大小写比较字符串
            static bool IEquals(std::string_view a, std::string_view b);
            
            // 空字符判定
            static bool IsBlank(const char c);
        };
    }
}
