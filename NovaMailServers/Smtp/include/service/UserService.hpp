#pragma once
#include <LikeHub/service/Service.hpp>
#include <address/EmailAddress.hpp>
#include <entity/User.hpp>
#include <string>
#include <string_view>

namespace NovaMail {
    namespace Smtp::Service {
        class UserService : public LikeHub::IService {
        public:
            explicit UserService(LikeHub::Storage::DatabasePool& pool);

            // 解析 用户状态 枚举
            static Entity::UserStatus ParseUserStatus(std::string_view s);

            // 获取用户 通过 ID
            bool GetUser(uint64_t userId, Entity::User& user);
            bool GetUserByAccountAuthCode(std::string_view account, std::string_view authCode, Entity::User& user);
            bool GetUserByMailboxAuthCode(std::string_view localPart, std::string_view domain, std::string_view authCode, Entity::User& user);
        };
    }
}
