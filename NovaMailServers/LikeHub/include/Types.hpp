#pragma once
#include <cstdint>
#include <cctype>
#include <string_view>

namespace LikeHub {
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
}
