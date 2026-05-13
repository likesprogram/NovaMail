#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace NovaMail::Smtp {
    struct MxRecord {
        std::string host;
        uint16_t preference = 0;
    };

    class DnsResolver {
    public:
        DnsResolver() = default;
        ~DnsResolver() = default;

        DnsResolver(const DnsResolver&) = default;
        DnsResolver& operator=(const DnsResolver&) = default;

        DnsResolver(DnsResolver&&) noexcept = default;
        DnsResolver& operator=(DnsResolver&&) noexcept = default;

        /**
         * 查询指定域名的 MX 记录。
         *
         * 返回结果按 preference 升序排序。
         * 如果 MX 查询失败，会按照 SMTP 规则尝试回退到域名本身。
         */
        std::vector<MxRecord> QueryMx(const std::string& domain) const;
    };

}