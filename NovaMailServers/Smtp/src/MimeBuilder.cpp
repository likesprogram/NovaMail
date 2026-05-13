#include <LikesProgram/net/Buffer.hpp>
#include <MimeBuilder.hpp>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace NovaMail {
    namespace Smtp::Mime {
        namespace {
            static char ToLowerAscii(char c) {
                return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }

            static bool IEquals(std::string_view a, std::string_view b) {
                if (a.size() != b.size()) {
                    return false;
                }

                for (size_t i = 0; i < a.size(); ++i) {
                    if (ToLowerAscii(a[i]) != ToLowerAscii(b[i])) {
                        return false;
                    }
                }

                return true;
            }

            static std::string Trim(std::string_view input) {
                size_t begin = 0;
                while (begin < input.size() &&
                       std::isspace(static_cast<unsigned char>(input[begin]))) {
                    ++begin;
                }

                size_t end = input.size();
                while (end > begin &&
                       std::isspace(static_cast<unsigned char>(input[end - 1]))) {
                    --end;
                }

                return std::string(input.substr(begin, end - begin));
            }

            static std::string_view TrimView(std::string_view input) {
                size_t begin = 0;
                while (begin < input.size() &&
                       std::isspace(static_cast<unsigned char>(input[begin]))) {
                    ++begin;
                }

                size_t end = input.size();
                while (end > begin &&
                       std::isspace(static_cast<unsigned char>(input[end - 1]))) {
                    --end;
                }

                return input.substr(begin, end - begin);
            }

            static bool IsAscii(std::string_view s) {
                for (unsigned char c : s) {
                    if (c >= 128) {
                        return false;
                    }
                }

                return true;
            }

            static size_t Utf8CharLength(unsigned char c) {
                if ((c & 0x80) == 0) {
                    return 1;
                }

                if ((c & 0xE0) == 0xC0) {
                    return 2;
                }

                if ((c & 0xF0) == 0xE0) {
                    return 3;
                }

                if ((c & 0xF8) == 0xF0) {
                    return 4;
                }

                return 1;
            }

            static std::vector<std::string_view> SplitUtf8Chunks(
                std::string_view input,
                size_t maxBytes
            ) {
                std::vector<std::string_view> chunks;

                size_t start = 0;
                size_t pos = 0;
                size_t currentBytes = 0;

                while (pos < input.size()) {
                    unsigned char c = static_cast<unsigned char>(input[pos]);
                    size_t len = Utf8CharLength(c);

                    if (pos + len > input.size()) {
                        len = 1;
                    }

                    if (currentBytes > 0 && currentBytes + len > maxBytes) {
                        chunks.emplace_back(input.substr(start, pos - start));
                        start = pos;
                        currentBytes = 0;
                    }

                    pos += len;
                    currentBytes += len;
                }

                if (start < input.size()) {
                    chunks.emplace_back(input.substr(start));
                }

                return chunks;
            }

            static bool NeedsQuotedString(std::string_view s) {
                for (unsigned char c : s) {
                    if (c == '"' || c == '\\' || c == ',' || c == ';' ||
                        c == '<' || c == '>' || c == '(' || c == ')' ||
                        c == '[' || c == ']' || c == ':' || c == '@') {
                        return true;
                    }
                }

                return false;
            }

            static std::string QuoteString(std::string_view s) {
                std::string out;
                out.reserve(s.size() + 8);

                out.push_back('"');

                for (char c : s) {
                    if (c == '"' || c == '\\') {
                        out.push_back('\\');
                    }

                    out.push_back(c);
                }

                out.push_back('"');
                return out;
            }

            static std::string HexByte(unsigned char c) {
                static constexpr char hex[] = "0123456789ABCDEF";

                std::string out;
                out.push_back('=');
                out.push_back(hex[(c >> 4) & 0x0F]);
                out.push_back(hex[c & 0x0F]);

                return out;
            }

            static bool IsQpPrintable(unsigned char c) {
                return (c >= 33 && c <= 60) || (c >= 62 && c <= 126);
            }

        }

        std::string MimeBuilder::BuildStandardMail(
            const LikesProgram::Net::Buffer& mailData,
            const BuildOptions& options
        ) {
            std::string_view rawMail(
                reinterpret_cast<const char*>(mailData.Peek()),
                mailData.ReadableBytes()
            );

            return BuildStandardMail(rawMail, options);
        }

        std::string MimeBuilder::BuildStandardMail(
            std::string_view rawMail,
            const BuildOptions& options
        ) {
            if (options.from.address.empty()) {
                throw std::invalid_argument(
                    "MimeBuilder::BuildStandardMail failed: from address is empty"
                );
            }

            if (options.to.empty() && options.cc.empty() && options.bcc.empty()) {
                throw std::invalid_argument(
                    "MimeBuilder::BuildStandardMail failed: no recipients"
                );
            }

            RawMailAnalyzeResult analyze = AnalyzeRawMail(rawMail);

            switch (analyze.kind) {
            case RawMailKind::Rfc5322Like:
                if (options.supplementMissingHeaders) {
                    return SupplementRfc5322Headers(rawMail, options, analyze);
                }

                return EnsureCrlf(rawMail);

            case RawMailKind::HeaderLikePartial:
                return SupplementRfc5322Headers(rawMail, options, analyze);

            case RawMailKind::BodyOnly:
            default:
                return BuildFromBodyOnly(rawMail, options);
            }
        }

        std::string MimeBuilder::EnsureCrlf(std::string_view input) {
            std::string out;
            out.reserve(input.size() + 16);

            for (size_t i = 0; i < input.size(); ++i) {
                char c = input[i];

                if (c == '\r') {
                    if (i + 1 < input.size() && input[i + 1] == '\n') {
                        out += "\r\n";
                        ++i;
                    } else {
                        out += "\r\n";
                    }
                } else if (c == '\n') {
                    out += "\r\n";
                } else {
                    out += c;
                }
            }

            return out;
        }

        bool MimeBuilder::HasHeader(std::string_view headerBlock, std::string_view name) {
            size_t pos = 0;

            while (pos < headerBlock.size()) {
                size_t lineEnd = headerBlock.find('\n', pos);
                if (lineEnd == std::string_view::npos) {
                    lineEnd = headerBlock.size();
                }

                std::string_view line = headerBlock.substr(pos, lineEnd - pos);

                if (!line.empty() && line.back() == '\r') {
                    line.remove_suffix(1);
                }

                // 折叠头行，不作为新 header
                if (!line.empty() && (line[0] == ' ' || line[0] == '\t')) {
                    if (lineEnd == headerBlock.size()) {
                        break;
                    }

                    pos = lineEnd + 1;
                    continue;
                }

                size_t colon = line.find(':');
                if (colon != std::string_view::npos) {
                    std::string_view key = TrimView(line.substr(0, colon));

                    if (IEquals(key, name)) {
                        return true;
                    }
                }

                if (lineEnd == headerBlock.size()) {
                    break;
                }

                pos = lineEnd + 1;
            }

            return false;
        }

        RawMailAnalyzeResult MimeBuilder::AnalyzeRawMail(std::string_view rawMail) {
            RawMailAnalyzeResult result;

            size_t sep = rawMail.find("\r\n\r\n");

            if (sep == std::string_view::npos) {
                sep = rawMail.find("\n\n");
            }

            if (sep == std::string_view::npos) {
                result.kind = RawMailKind::BodyOnly;
                return result;
            }

            result.hasHeaderBodySeparator = true;

            std::string_view headers = rawMail.substr(0, sep);

            result.hasFrom = HasHeader(headers, "From");
            result.hasTo = HasHeader(headers, "To");
            result.hasCc = HasHeader(headers, "Cc");
            result.hasSubject = HasHeader(headers, "Subject");
            result.hasDate = HasHeader(headers, "Date");
            result.hasMessageId = HasHeader(headers, "Message-ID");
            result.hasMimeVersion = HasHeader(headers, "MIME-Version");
            result.hasContentType = HasHeader(headers, "Content-Type");
            result.hasTransferEncoding = HasHeader(headers, "Content-Transfer-Encoding");

            /*
             * 本地发信场景下：
             * - 有 From + Date，基本认为是 RFC5322-like；
             * - 有分隔符但缺 From/Date，则认为是半成品邮件，需要补头。
             */
            if (result.hasFrom && result.hasDate) {
                result.kind = RawMailKind::Rfc5322Like;
            } else {
                result.kind = RawMailKind::HeaderLikePartial;
            }

            return result;
        }

        bool MimeBuilder::LooksLikeRfc5322Message(std::string_view rawMail) {
            return AnalyzeRawMail(rawMail).kind == RawMailKind::Rfc5322Like;
        }

        std::string MimeBuilder::FormatRfc5322Date() {
            std::time_t now = std::time(nullptr);

            std::tm tm{};
    #if defined(_WIN32)
            gmtime_s(&tm, &now);
    #else
            gmtime_r(&now, &tm);
    #endif

            char buf[128];

            std::strftime(
                buf,
                sizeof(buf),
                "%a, %d %b %Y %H:%M:%S +0000",
                &tm
            );

            return buf;
        }

        std::string MimeBuilder::BuildMessageId(uint64_t mailId, std::string_view domain) {
            using namespace std::chrono;

            std::string safeDomain = Trim(domain);
            if (safeDomain.empty()) {
                safeDomain = "localhost";
            }

            if (mailId != 0) {
                return "<" +
                       std::to_string(mailId) +
                       "." +
                       std::to_string(static_cast<uint64_t>(std::time(nullptr))) +
                       "@" +
                       safeDomain +
                       ">";
            }

            auto now = system_clock::now();
            auto ns = duration_cast<nanoseconds>(now.time_since_epoch()).count();

            std::random_device rd;
            std::mt19937_64 rng(rd());

            return "<" +
                   std::to_string(static_cast<int64_t>(ns)) +
                   "." +
                   std::to_string(rng()) +
                   "@" +
                   safeDomain +
                   ">";
        }

        std::string MimeBuilder::Base64Encode(const unsigned char* data, size_t size) {
            static constexpr char table[] =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

            std::string out;
            out.reserve(((size + 2) / 3) * 4);

            for (size_t i = 0; i < size; i += 3) {
                uint32_t value = 0;
                int bytes = 0;

                for (int j = 0; j < 3; ++j) {
                    value <<= 8;

                    if (i + static_cast<size_t>(j) < size) {
                        value |= data[i + static_cast<size_t>(j)];
                        ++bytes;
                    }
                }

                const int pad = 3 - bytes;

                for (int j = 0; j < 4 - pad; ++j) {
                    const int shift = 18 - j * 6;
                    out.push_back(table[(value >> shift) & 0x3F]);
                }

                for (int j = 0; j < pad; ++j) {
                    out.push_back('=');
                }
            }

            return out;
        }

        std::string MimeBuilder::Base64Encode(std::string_view data) {
            return Base64Encode(
                reinterpret_cast<const unsigned char*>(data.data()),
                data.size()
            );
        }

        std::string MimeBuilder::EncodeHeaderText(std::string_view text) {
            if (text.empty()) {
                return "";
            }

            if (IsAscii(text)) {
                return std::string(text);
            }

            /*
             * RFC2047 encoded-word 单段不要太长。
             * 这里按 UTF-8 字符边界切分，避免中文被切坏。
             */
            std::vector<std::string_view> chunks = SplitUtf8Chunks(text, 42);

            std::string out;

            for (size_t i = 0; i < chunks.size(); ++i) {
                if (i != 0) {
                    out += " ";
                }

                out += "=?UTF-8?B?";
                out += Base64Encode(chunks[i]);
                out += "?=";
            }

            return out;
        }

        std::string MimeBuilder::FormatAddress(const MailAddress& address) {
            if (address.address.empty()) {
                return "";
            }

            if (address.displayName.empty()) {
                return "<" + address.address + ">";
            }

            std::string name;

            if (!IsAscii(address.displayName)) {
                name = EncodeHeaderText(address.displayName);
            } else if (NeedsQuotedString(address.displayName)) {
                name = QuoteString(address.displayName);
            } else {
                name = address.displayName;
            }

            return name + " <" + address.address + ">";
        }

        std::string MimeBuilder::JoinAddressList(const std::vector<MailAddress>& addresses) {
            std::string out;

            for (const auto& address : addresses) {
                if (address.address.empty()) {
                    continue;
                }

                if (!out.empty()) {
                    out += ", ";
                }

                out += FormatAddress(address);
            }

            return out;
        }

        std::string MimeBuilder::FoldHeaderLine(
            std::string_view name,
            std::string_view value,
            size_t softLimit
        ) {
            std::string line;
            line.reserve(name.size() + value.size() + 4);

            line.append(name);
            line += ": ";
            line.append(value);

            if (line.size() <= softLimit) {
                return line + "\r\n";
            }

            std::string out;
            out.reserve(line.size() + line.size() / softLimit * 4);

            size_t pos = 0;
            size_t currentLimit = softLimit;

            while (pos < line.size()) {
                size_t remaining = line.size() - pos;

                if (remaining <= currentLimit) {
                    out.append(line, pos, remaining);
                    break;
                }

                size_t breakPos = std::string::npos;
                size_t searchEnd = pos + currentLimit;

                if (searchEnd >= line.size()) {
                    searchEnd = line.size() - 1;
                }

                for (size_t i = searchEnd; i > pos; --i) {
                    char c = line[i - 1];

                    if (c == ' ' || c == ',' || c == ';') {
                        breakPos = i;
                        break;
                    }
                }

                if (breakPos == std::string::npos || breakPos <= pos) {
                    breakPos = pos + currentLimit;
                }

                out.append(line, pos, breakPos - pos);
                out += "\r\n\t";

                pos = breakPos;

                while (pos < line.size() && line[pos] == ' ') {
                    ++pos;
                }

                currentLimit = softLimit - 1;
            }

            out += "\r\n";
            return out;
        }

        std::string MimeBuilder::QuotedPrintableEncode(std::string_view input) {
            std::string normalized = EnsureCrlf(input);

            std::string out;
            out.reserve(normalized.size() + normalized.size() / 8);

            size_t pos = 0;

            while (pos < normalized.size()) {
                size_t lineEnd = normalized.find("\r\n", pos);
                bool hasNewline = lineEnd != std::string::npos;

                if (!hasNewline) {
                    lineEnd = normalized.size();
                }

                std::string_view line =
                    std::string_view(normalized).substr(pos, lineEnd - pos);

                size_t col = 0;

                for (size_t i = 0; i < line.size(); ++i) {
                    unsigned char c = static_cast<unsigned char>(line[i]);
                    bool isLastChar = i + 1 == line.size();

                    std::string token;

                    if ((c == ' ' || c == '\t') && !isLastChar) {
                        token.push_back(static_cast<char>(c));
                    } else if ((c == ' ' || c == '\t') && isLastChar) {
                        token = HexByte(c);
                    } else if (IsQpPrintable(c)) {
                        token.push_back(static_cast<char>(c));
                    } else {
                        token = HexByte(c);
                    }

                    if (col + token.size() > 75) {
                        out += "=\r\n";
                        col = 0;
                    }

                    out += token;
                    col += token.size();
                }

                if (hasNewline) {
                    out += "\r\n";
                    pos = lineEnd + 2;
                } else {
                    break;
                }
            }

            return out;
        }

        std::string MimeBuilder::BuildTextPart(
            std::string_view contentType,
            std::string_view text
        ) {
            std::string part;

            part += FoldHeaderLine("Content-Type", contentType);
            part += "Content-Transfer-Encoding: quoted-printable\r\n";
            part += "\r\n";
            part += QuotedPrintableEncode(text);
            part += "\r\n";

            return part;
        }

        bool MimeBuilder::IsHtmlBody(std::string_view body) {
            std::string sample;
            sample.reserve(std::min<size_t>(body.size(), 512));

            size_t n = std::min<size_t>(body.size(), 512);

            for (size_t i = 0; i < n; ++i) {
                sample.push_back(ToLowerAscii(body[i]));
            }

            return sample.find("<html") != std::string::npos ||
                   sample.find("<body") != std::string::npos ||
                   sample.find("<div") != std::string::npos ||
                   sample.find("<p") != std::string::npos ||
                   sample.find("<br") != std::string::npos ||
                   sample.find("</") != std::string::npos;
        }

        bool MimeBuilder::IsForbiddenExtraHeader(std::string_view name) {
            return IEquals(name, "From") ||
                   IEquals(name, "To") ||
                   IEquals(name, "Cc") ||
                   IEquals(name, "Bcc") ||
                   IEquals(name, "Subject") ||
                   IEquals(name, "Date") ||
                   IEquals(name, "Message-ID") ||
                   IEquals(name, "MIME-Version") ||
                   IEquals(name, "Content-Type") ||
                   IEquals(name, "Content-Transfer-Encoding") ||
                   IEquals(name, "DKIM-Signature");
        }

        std::string MimeBuilder::BuildFromBodyOnly(
            std::string_view body,
            const BuildOptions& options
        ) {
            BodyKind bodyKind = options.bodyKind;

            if (bodyKind == BodyKind::Auto) {
                bodyKind = IsHtmlBody(body) ? BodyKind::Html : BodyKind::PlainText;
            }

            std::string out;

            out += FoldHeaderLine("From", FormatAddress(options.from));

            if (!options.to.empty()) {
                out += FoldHeaderLine("To", JoinAddressList(options.to));
            }

            if (!options.cc.empty()) {
                out += FoldHeaderLine("Cc", JoinAddressList(options.cc));
            }

            /*
             * Bcc 不写入邮件头。
             * Bcc 只进入 SMTP envelope recipients。
             */

            out += FoldHeaderLine("Subject", EncodeHeaderText(options.subject));
            out += FoldHeaderLine("Date", FormatRfc5322Date());

            out += FoldHeaderLine(
                "Message-ID",
                BuildMessageId(options.mailId, options.messageIdDomain)
            );

            out += "MIME-Version: 1.0\r\n";

            for (const auto& [key, value] : options.extraHeaders) {
                if (key.empty() || IsForbiddenExtraHeader(key)) {
                    continue;
                }

                out += FoldHeaderLine(key, value);
            }

            if (bodyKind == BodyKind::Html) {
                out += BuildTextPart("text/html; charset=UTF-8", body);
            } else {
                out += BuildTextPart("text/plain; charset=UTF-8", body);
            }

            return EnsureCrlf(out);
        }

        std::string MimeBuilder::SupplementRfc5322Headers(
            std::string_view rawMail,
            const BuildOptions& options,
            const RawMailAnalyzeResult& analyze
        ) {
            size_t sep = rawMail.find("\r\n\r\n");

            if (sep == std::string_view::npos) sep = rawMail.find("\n\n");

            if (sep == std::string_view::npos) {
                return BuildFromBodyOnly(rawMail, options);
            }

            std::string prefix;

            if (!analyze.hasFrom) {
                prefix += FoldHeaderLine("From", FormatAddress(options.from));
            }

            if (!analyze.hasTo && !options.to.empty()) {
                prefix += FoldHeaderLine("To", JoinAddressList(options.to));
            }

            if (!analyze.hasCc && !options.cc.empty()) {
                prefix += FoldHeaderLine("Cc", JoinAddressList(options.cc));
            }

            if (!analyze.hasSubject) {
                prefix += FoldHeaderLine("Subject", EncodeHeaderText(options.subject));
            }

            if (!analyze.hasDate) {
                prefix += FoldHeaderLine("Date", FormatRfc5322Date());
            }

            if (!analyze.hasMessageId) {
                prefix += FoldHeaderLine(
                    "Message-ID",
                    BuildMessageId(options.mailId, options.messageIdDomain)
                );
            }

            if (!analyze.hasMimeVersion) {
                prefix += "MIME-Version: 1.0\r\n";
            }

            if (!analyze.hasContentType) {
                prefix += FoldHeaderLine("Content-Type", options.defaultContentType);
            }

            if (!analyze.hasTransferEncoding) {
                prefix += FoldHeaderLine(
                    "Content-Transfer-Encoding",
                    options.defaultTransferEncoding
                );
            }

            for (const auto& [key, value] : options.extraHeaders) {
                if (key.empty() || IsForbiddenExtraHeader(key)) continue;

                /*
                 * 已有邮件头里没有这个额外头，才补。
                 */
                std::string_view headerBlock = rawMail.substr(0, sep);
                if (!HasHeader(headerBlock, key)) {
                    prefix += FoldHeaderLine(key, value);
                }
            }

            std::string out;
            out.reserve(prefix.size() + rawMail.size() + 16);

            out += prefix;
            out.append(rawMail.data(), rawMail.size());

            return EnsureCrlf(out);
        }
    }
}