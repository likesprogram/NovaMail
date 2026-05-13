#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace LikesProgram {
    namespace Net {
        class Buffer;
    }
}

namespace NovaMail {
    namespace Smtp::Mime {
        struct MailAddress {
            std::string address;
            std::string displayName;

            MailAddress() = default;

            explicit MailAddress(std::string addr)
                : address(std::move(addr)) {
            }

            MailAddress(std::string addr, std::string name)
                : address(std::move(addr))
                , displayName(std::move(name)) {
            }

            bool Empty() const {
                return address.empty();
            }
        };

        enum class BodyKind {
            Auto = 0,
            PlainText,
            Html
        };

        enum class RawMailKind {
            BodyOnly = 0,
            HeaderLikePartial,
            Rfc5322Like
        };

        struct BuildOptions {
            uint64_t mailId = 0;

            MailAddress from;

            std::vector<MailAddress> to;
            std::vector<MailAddress> cc;
            std::vector<MailAddress> bcc;

            std::string subject;

            // Message-ID 生成用域名，例如 lby.email
            std::string messageIdDomain = "localhost";

            // Buffer 只有正文时，用它判断正文类型
            BodyKind bodyKind = BodyKind::Auto;

            // 已有邮件缺少 Content-Type 时使用
            std::string defaultContentType = "text/plain; charset=UTF-8";

            // 已有邮件缺少 Content-Transfer-Encoding 时使用
            std::string defaultTransferEncoding = "8bit";

            // 是否给不完整邮件补齐基础头
            bool supplementMissingHeaders = true;

            // 额外头，例如 X-Mailer、X-NovaMail-Id
            std::map<std::string, std::string> extraHeaders;
        };

        struct RawMailAnalyzeResult {
            RawMailKind kind = RawMailKind::BodyOnly;

            bool hasHeaderBodySeparator = false;
            bool hasFrom = false;
            bool hasTo = false;
            bool hasCc = false;
            bool hasSubject = false;
            bool hasDate = false;
            bool hasMessageId = false;
            bool hasMimeVersion = false;
            bool hasContentType = false;
            bool hasTransferEncoding = false;
        };

        class MimeBuilder {
        public:
            // 核心接口：直接接收 Buffer，返回标准 RFC5322/MIME 邮件字符串
            static std::string BuildStandardMail(
                const LikesProgram::Net::Buffer& mailData,
                const BuildOptions& options
            );

            // 接收 string_view，方便测试
            static std::string BuildStandardMail(
                std::string_view rawMail,
                const BuildOptions& options
            );

            // 只做 CRLF 规范化，不做 dot-stuffing
            static std::string EnsureCrlf(std::string_view input);

            // 分析 Buffer/raw 内容是完整邮件、不完整邮件还是纯正文
            static RawMailAnalyzeResult AnalyzeRawMail(std::string_view rawMail);

            static bool LooksLikeRfc5322Message(std::string_view rawMail);

        private:
            static std::string BuildFromBodyOnly(
                std::string_view body,
                const BuildOptions& options
            );

            static std::string SupplementRfc5322Headers(
                std::string_view rawMail,
                const BuildOptions& options,
                const RawMailAnalyzeResult& analyze
            );

            static std::string BuildTextPart(
                std::string_view contentType,
                std::string_view text
            );

            static std::string FormatRfc5322Date();
            static std::string BuildMessageId(uint64_t mailId, std::string_view domain);

            static std::string FormatAddress(const MailAddress& address);
            static std::string JoinAddressList(const std::vector<MailAddress>& addresses);

            static std::string FoldHeaderLine(
                std::string_view name,
                std::string_view value,
                size_t softLimit = 78
            );

            static std::string EncodeHeaderText(std::string_view text);

            static std::string Base64Encode(const unsigned char* data, size_t size);
            static std::string Base64Encode(std::string_view data);

            static std::string QuotedPrintableEncode(std::string_view input);

            static bool HasHeader(std::string_view headerBlock, std::string_view name);
            static bool IsHtmlBody(std::string_view body);
            static bool IsForbiddenExtraHeader(std::string_view name);
        };

    }
}