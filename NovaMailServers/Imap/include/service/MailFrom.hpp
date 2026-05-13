#pragma once
#include <address/EmailAddress.hpp>
#include <entity/MailBox.hpp>

namespace NovaMail {
    namespace Imap::Types {
        enum class MailSourceType {
            Unknown = 0,
            // 本系统认证用户发出的邮件
            Local,
            // 外部服务器投递进来的邮件
            External,
            // MAIL FROM:<>，退信、DSN、系统回执常见
            NullReversePath
        };
        class MailFrom {
        public:
            MailFrom();
            
            bool HasAddress() const noexcept { return !m_addressString.empty(); }
            const std::string& AddressString() const noexcept { return m_addressString; }

            Entity::MailBox MailBox() const noexcept;
            Address::EmailAddress Address() const noexcept;
            std::string_view DisplayName() const noexcept;
            size_t MailSize() const noexcept;
            bool NullReversePath() const noexcept;
            bool IsLocalSource() const noexcept;
            bool IsExternalSource() const noexcept;
            bool IsNullReversePath() const noexcept;

            void SetMailBox(const Entity::MailBox& mailbox);
            void SetAddress(const Address::EmailAddress& address);
            void SetDisplayName(std::string_view displayName);
            void SetSize(size_t mailSize) noexcept;
            void SetBody(std::string_view body);
            void SetSourceType(MailSourceType type);
            void SetNullReversePath(bool value);
            
            void Reset();
        private:
            Entity::MailBox m_mailbox;
            Address::EmailAddress m_address;
            bool m_nullReversePath = false;
            std::string m_addressString;
            std::string m_displayName;                        // 可选
            size_t m_mailSize = 0;                            // 可选
            MailSourceType m_sourceType = MailSourceType::Unknown;
        };
    }
}