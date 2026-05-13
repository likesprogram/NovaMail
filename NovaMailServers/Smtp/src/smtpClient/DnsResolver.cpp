#include <smtpClient/DnsResolver.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <LikesProgram/String.hpp>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <arpa/nameser.h>
#include <netdb.h>
#include <resolv.h>

namespace NovaMail::Smtp {
    std::vector<MxRecord> DnsResolver::QueryMx(const std::string& domain) const {
        std::vector<MxRecord> records;

        if (domain.empty()) {
            LogWarn(u"DNS MX query skipped: reason=EmptyDomain");
            return records;
        }

        unsigned char answer[NS_PACKETSZ];

        int len = res_query(
            domain.c_str(),
            ns_c_in,
            ns_t_mx,
            answer,
            sizeof(answer)
        );

        if (len < 0) {
            LogWarn(
                u"DNS MX query failed: domain={} h_errno={} fallbackHost={}",
                LikesProgram::String(domain),
                h_errno,
                LikesProgram::String(domain)
            );

            // SMTP 允许域名没有 MX 记录时，回退尝试域名本身。
            records.push_back(MxRecord{
                .host = domain,
                .preference = 65535
            });

            return records;
        }

        ns_msg handle;

        if (ns_initparse(answer, len, &handle) < 0) {
            LogWarn(
                u"DNS MX parse failed: domain={} errno={} error={}",
                LikesProgram::String(domain),
                errno,
                LikesProgram::String(std::strerror(errno))
            );

            return records;
        }

        int answerCount = ns_msg_count(handle, ns_s_an);

        for (int i = 0; i < answerCount; ++i) {
            ns_rr rr;

            if (ns_parserr(&handle, ns_s_an, i, &rr) < 0) {
                LogWarn(
                    u"DNS MX answer parse failed: domain={} index={} errno={} error={}",
                    LikesProgram::String(domain),
                    static_cast<int64_t>(i),
                    errno,
                    LikesProgram::String(std::strerror(errno))
                );

                continue;
            }

            if (ns_rr_type(rr) != ns_t_mx) {
                continue;
            }

            const unsigned char* rdata = ns_rr_rdata(rr);
            if (!rdata) {
                LogWarn(
                    u"DNS MX record ignored: domain={} index={} reason=EmptyRdata",
                    LikesProgram::String(domain),
                    static_cast<int64_t>(i)
                );

                continue;
            }

            uint16_t preference = ns_get16(rdata);

            char exchange[NS_MAXDNAME];

            int expandResult = dn_expand(
                answer,
                answer + len,
                rdata + 2,
                exchange,
                sizeof(exchange)
            );

            if (expandResult < 0) {
                LogWarn(
                    u"DNS MX exchange expand failed: domain={} index={} preference={}",
                    LikesProgram::String(domain),
                    static_cast<int64_t>(i),
                    static_cast<int64_t>(preference)
                );

                continue;
            }

            std::string host = exchange;

            if (!host.empty() && host.back() == '.') {
                host.pop_back();
            }

            if (host.empty()) {
                LogWarn(
                    u"DNS MX record ignored: domain={} index={} preference={} reason=EmptyHost",
                    LikesProgram::String(domain),
                    static_cast<int64_t>(i),
                    static_cast<int64_t>(preference)
                );

                continue;
            }

            records.push_back(MxRecord{
                .host = host,
                .preference = preference
            });
        }

        std::stable_sort(
            records.begin(),
            records.end(),
            [](const MxRecord& lhs, const MxRecord& rhs) {
                return lhs.preference < rhs.preference;
            }
        );

        if (records.empty()) {
            LogWarn(
                u"DNS MX query returned no usable records: domain={} fallbackHost={}",
                LikesProgram::String(domain),
                LikesProgram::String(domain)
            );

            records.push_back(MxRecord{
                .host = domain,
                .preference = 65535
            });
        }

        LogDebug(
            u"DNS MX query success: domain={} recordCount={}",
            LikesProgram::String(domain),
            static_cast<int64_t>(records.size())
        );

        return records;
    }
}