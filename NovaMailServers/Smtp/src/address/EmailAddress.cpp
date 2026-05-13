#include <address/EmailAddress.hpp>

namespace NovaMail {
    namespace Smtp::Address {static bool IsAtext(unsigned char c) noexcept {
            if (std::isalnum(c)) return true;
            switch (c) {
            case '!': case '#': case '$': case '%': case '&': case '\'': case '*':
            case '+': case '-': case '/': case '=': case '?': case '^': case '_':
            case '`': case '{': case '|': case '}': case '~': return true;
            default: return false;
            }
        }

        static bool ValidateLocal(std::string_view local) {
            if (local.empty()) return false;
            if (local.size() > 64) return false;

            bool prevDot = false;
            for (size_t i = 0; i < local.size(); ++i) {
                const unsigned char c = static_cast<unsigned char>(local[i]);
                if (c == '.') { // local-part 不能以 . 开头、不能以 . 结尾、不能连续 ..
                    if (i == 0 || i + 1 == local.size() || prevDot) return false;
                    prevDot = true;
                    continue;
                }
                if (!IsAtext(c)) return false;
                prevDot = false;
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

        LikesProgram::Configuration EmailAddress::ToConfiguration() const {
            return LikesProgram::Configuration(
                LikesProgram::String(ToString())
            );
        }

        EmailAddress EmailAddress::FromConfiguration(const LikesProgram::Configuration& cfg) {
            std::string addressText;

            try {
                addressText = cfg
                    .AsString(LikesProgram::Configuration::CastPolicy::AutoConvert)
                    .ToStdString();
            } catch (const std::exception& e) {
                throw std::runtime_error(
                    std::string("EmailAddress configuration cannot cast to string: ") + e.what()
                );
            }

            EmailAddress address;

            if (!EmailAddress::Parse(addressText, address)) {
                throw std::runtime_error("Invalid email address configuration: " + addressText);
            }

            return address;
        }

        bool EmailAddress::ParseConfiguration(
            const LikesProgram::Configuration& cfg,
            EmailAddress& out
        ) noexcept {
            try {
                out = EmailAddress::FromConfiguration(cfg);
                return true;
            } catch (...) {
                out.Reset();
                return false;
            }
        }

        void EmailAddress::Reset() {
            m_local.clear();
            m_domain.Reset();
        }
    }
}