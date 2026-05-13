#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <types/RcptNotify.hpp>
#include <address/EmailAddress.hpp>
#include <entity/MailBox.hpp>
#include <LikesProgram/Configuration.hpp>

namespace NovaMail {
    namespace Smtp::Types {
        enum class RcptTargetType {
            Unknown = 0,
            // 收件人属于本系统托管域名
            Local,
            // 收件人不属于本系统，需要外发
            Outbound
        };
        class RcptTo {
        public:
            RcptTo();

            bool HasAddress() const noexcept { return !m_addressString.empty(); }
            const std::string& AddressString() const noexcept { return m_addressString; }

            Entity::MailBox MailBox() const noexcept;
            Address::EmailAddress Address() const noexcept;
            std::string_view DisplayName() const noexcept;

            RcptNotify Notify() const noexcept;
            std::string_view OrCpt() const noexcept;

            const std::unordered_map<std::string,std::string>& Extensions() const noexcept;
            bool IsLocalTarget() const noexcept;
            bool IsOutboundTarget() const noexcept;

            void SetMailBox(const Entity::MailBox& mailbox);
            void SetAddress(const Address::EmailAddress& address);
            void SetDisplayName(std::string_view displayName);

            void SetNotify(RcptNotify notify) noexcept;
            void AddNotify(RcptNotify notify) noexcept;

            void SetOrCpt(std::string_view orcpt);

            void SetExtension(std::string_view key,std::string_view value);
            void SetTargetType(RcptTargetType type);

                        // 转 Configuration
            LikesProgram::Configuration ToConfiguration() const;

            // 从 Configuration 构造，失败抛异常
            static RcptTo FromConfiguration(const LikesProgram::Configuration& cfg);

            // 从 Configuration 构造，失败返回 false
            static bool ParseConfiguration(const LikesProgram::Configuration& cfg, RcptTo& out) noexcept;

            void Reset();
        private:
            Entity::MailBox m_mailbox;
            Address::EmailAddress m_address;
            std::string m_addressString;
            std::string m_displayName;
            RcptNotify m_notify = RcptNotify::None;
            std::string m_orcpt;
            std::unordered_map<std::string,std::string> m_extensions;
            RcptTargetType m_targetType = RcptTargetType::Unknown;
        };
    }
}