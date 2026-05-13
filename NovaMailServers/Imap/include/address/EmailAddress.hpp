#pragma once
#include <string>
#include <address/Domain.hpp>

namespace NovaMail {
    namespace Imap::Address {
        class EmailAddress {
        public:
            static bool Parse(std::string_view input, EmailAddress& out);

            const std::string& Local() const;
            const Domain& GetDomain() const;

            std::string ToString() const;

            void Reset();
        private:
            std::string m_local;
            Domain m_domain;
        };
    }
}
