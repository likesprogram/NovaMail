#pragma once
#include <string>
#include <address/Domain.hpp>
#include <LikesProgram/Configuration.hpp>

namespace NovaMail {
    namespace Smtp::Address {
        class EmailAddress {
        public:
            static bool Parse(std::string_view input, EmailAddress& out);

            const std::string& Local() const;
            const Domain& GetDomain() const;

            std::string ToString() const;
            
            // 转 Configuration
            LikesProgram::Configuration ToConfiguration() const;

            // 从 Configuration 构造，失败抛异常
            static EmailAddress FromConfiguration(const LikesProgram::Configuration& cfg);

            // 从 Configuration 构造，失败返回 false
            static bool ParseConfiguration(const LikesProgram::Configuration& cfg, EmailAddress& out) noexcept;
            
            void Reset();
        private:
            std::string m_local;
            Domain m_domain;
        };
    }
}
