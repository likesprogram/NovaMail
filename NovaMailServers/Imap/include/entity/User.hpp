#pragma once
#include <cstdint>
#include <string>

namespace NovaMail {
    namespace Imap::Entity {
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

            // 当前 IMAP 会话绑定的邮箱
            uint64_t mailBoxId{0};
            std::string localPart;
            std::string domainName;
            std::string mailAddress;

            bool Valid() const noexcept {
                return userId != 0;
            }
            bool HasMailBox() const noexcept {
                return mailBoxId != 0;
            }
        };
    }
}
