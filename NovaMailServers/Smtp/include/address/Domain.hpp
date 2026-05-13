#pragma once
#include <string>
#include <string_view>
#include <LikesProgram/Configuration.hpp>

namespace NovaMail {
    namespace Smtp::Address {
        class Domain {
        public:
            Domain() = default;
            explicit Domain(std::string name);

            // 创建并验证
            static bool Parse(std::string_view text, Domain& out);

            // 语法验证
            static bool IsValid(std::string_view text);

            // 获取域名
            const std::string& Name() const noexcept;

            // 是否为空
            bool Empty() const noexcept;

            // 转字符串
            std::string ToString() const;

            // 转 Configuration
            LikesProgram::Configuration ToConfiguration() const;

            // 从 Configuration 构造，失败抛异常
            static Domain FromConfiguration(const LikesProgram::Configuration& cfg);

            // 从 Configuration 构造，失败返回 false
            static bool ParseConfiguration(const LikesProgram::Configuration& cfg, Domain& out) noexcept;

            // 比较
            bool operator==(const Domain& other) const noexcept;
            bool operator!=(const Domain& other) const noexcept;

            void Reset();
        private:
            std::string m_name;
        };
    }
}
