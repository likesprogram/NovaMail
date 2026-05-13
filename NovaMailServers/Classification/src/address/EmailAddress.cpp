#include <address/EmailAddress.hpp>

namespace NovaMail {
    namespace Imap::Address {
        static bool ValidateLocal(std::string_view local)
        {
            if (local.empty()) return false;
            if (local.size() > 64) return false;
            for (char c : local) {
                if (!(std::isalnum((unsigned char)c) || c == '.' || c == '_' || c == '-' || c == '+')) return false;
            }
            return true;
        }

        bool EmailAddress::Parse(std::string_view input, EmailAddress& out) {
            if (input.empty()) return false;

            // 支持 <user@domain>
            if (input.front() == '<' && input.back() == '>') input = input.substr(1, input.size() - 2);

            auto at = input.find('@');
            if (at == std::string_view::npos) return false;

            auto local = input.substr(0, at);
            auto domain = input.substr(at + 1);

            if (!ValidateLocal(local)) return false;

            Domain dom;
            if (!Domain::Parse(domain, dom)) return false;

            out.m_local.assign(local);
            out.m_domain = std::move(dom);

            return true;
        }

        const std::string& EmailAddress::Local() const {
            return m_local;
        }

        const Domain& EmailAddress::GetDomain() const {
            return m_domain;
        }

        std::string EmailAddress::ToString() const {
            return m_local + "@" + m_domain.Name();
        }

        void EmailAddress::Reset() {
            m_local.clear();
            m_domain.Reset();
        }
    }
}