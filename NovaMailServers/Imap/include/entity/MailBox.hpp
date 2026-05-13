#pragma once
#include <entity/User.hpp>
#include <cstdint>
#include <string>

namespace NovaMail {
    namespace Imap::Entity {
        struct MailBox {
            uint64_t mailBoxId = 0;
            uint64_t userId = 0;
            uint64_t domainNameId = 0;

            std::string localPart;

            bool enabled = false;
            
            UserStatus userStatus = UserStatus::Normal;

            bool Valid() const noexcept {
                return mailBoxId != 0;
            }
        };
    }
}
