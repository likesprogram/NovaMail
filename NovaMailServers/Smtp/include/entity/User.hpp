#pragma once
#include <cstdint>
#include <string>

namespace NovaMail {
    namespace Smtp::Entity {
        enum class UserStatus : uint8_t {
            Normal = 0,
            Disabled,
            Deleted
        };

        struct User {
            uint64_t userId = 0;
            std::string account;
            std::string nickname;

            bool isAdmin = false;
            UserStatus status = UserStatus::Normal;

            bool Valid() const noexcept {
                return userId != 0;
            }
        };
    }
}
