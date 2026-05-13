#include <address/Domain.hpp>

namespace NovaMail {
    namespace Imap::Address {
        static bool ValidateLabel(std::string_view label) {
            if (label.empty()) return false;
            if (label.size() > 63) return false;
            if (label.front() == '-' || label.back() == '-') return false;
            for (char c : label) if (!(std::isalnum((unsigned char)c) || c == '-')) return false;
            return true;
        }

        Domain::Domain(std::string name) : m_name(std::move(name)) { }

        bool Domain::Parse(std::string_view text, Domain& out) {
            if (!IsValid(text)) return false;
        
            std::string name(text);
        
            // 域名统一小写
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return std::tolower(c); });
        
            out = Domain(std::move(name));
            return true;
        }

        bool Domain::IsValid(std::string_view text) {
            if (text.empty()) return false;
            if (text.size() > 255) return false;
        
            size_t start = 0;
        
            while (start < text.size()) {
                size_t end = text.find('.', start);
                if (end == std::string_view::npos) end = text.size();
            
                auto label = text.substr(start, end - start);
                if (!ValidateLabel(label)) return false;
            
                start = end + 1;
            }
            return true;
        }

        const std::string& Domain::Name() const noexcept {
            return m_name;
        }

        bool Domain::Empty() const noexcept {
            return m_name.empty();
        }

        std::string Domain::ToString() const {
            return m_name;
        }

        bool Domain::operator==(const Domain& other) const noexcept {
            return m_name == other.m_name;
        }

        bool Domain::operator!=(const Domain& other) const noexcept {
            return !(*this == other);
        }

        void Domain::Reset() {
            m_name.clear();
        }
    }
}
