#include <service/MailFrom.hpp>

namespace NovaMail {
    namespace Imap::Types {
        MailFrom::MailFrom() { Reset(); }

        Entity::MailBox MailFrom::MailBox() const noexcept {
            return m_mailbox;
        }

        Address::EmailAddress MailFrom::Address() const noexcept {
            return m_address;
        }
        
        std::string_view MailFrom::DisplayName() const noexcept {
            return m_displayName;
        }

        size_t MailFrom::MailSize() const noexcept {
            return m_mailSize;
        }

        bool MailFrom::NullReversePath() const noexcept {
            return m_nullReversePath;
        }

        bool MailFrom::IsLocalSource() const noexcept {
            return m_sourceType == MailSourceType::Local;
        }
    
        bool MailFrom::IsExternalSource() const noexcept {
            return m_sourceType == MailSourceType::External;
        }
    
        bool MailFrom::IsNullReversePath() const noexcept {
            return m_sourceType == MailSourceType::NullReversePath;
        }

        void MailFrom::SetMailBox(const Entity::MailBox& mailbox) {
            m_mailbox = mailbox;
        }

        void MailFrom::SetAddress(const Address::EmailAddress& address) {
            m_address = address;
            m_addressString = address.ToString();
        }

        void MailFrom::SetDisplayName(std::string_view displayName) {
            m_displayName.assign(displayName.data(), displayName.size());
        }

        void MailFrom::SetSize(size_t mailSize) noexcept {
            m_mailSize = mailSize;
        }
        
        void MailFrom::SetNullReversePath(bool value) {
            m_nullReversePath = value;
        }
        
        void MailFrom::SetSourceType(MailSourceType type) {
            m_sourceType = type;
        }

        void MailFrom::Reset() {
            m_address.Reset();
            m_addressString.clear();
            m_displayName.clear();
            m_mailSize = 0;
        }
    }
}