#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <LikesProgram/net/Buffer.hpp>

namespace LikeHub {
    struct CaseInsensitiveHash {
        size_t operator()(const std::string& s) const {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                [](unsigned char c){ return std::tolower(c); });
            return std::hash<std::string>()(lower);
        }
    };

    struct CaseInsensitiveEq {
        bool operator()(const std::string& a, const std::string& b) const {
            if (a.size() != b.size()) return false;

            for (size_t i = 0; i < a.size(); ++i) {
                if (std::tolower((unsigned char)a[i]) !=
                    std::tolower((unsigned char)b[i]))
                    return false;
            }
            return true;
        }
    };

    static inline std::string ToLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c){ return std::tolower(c); });
        return s;
    }

    struct MimeHeader {
        std::string key;
        std::string value;
    };

    struct MimePart {
        // 原始头
        std::unordered_map<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEq> headers;

        // 解析字段（cache）
        std::string contentType;     // text/plain
        std::string charset;         // utf-8
        std::string transferEncoding;// base64 / quoted-printable

        // 内容
        std::string rawBody;         // 未 decode
        std::string decodedBody;     // decode 后（UTF8）

        // 子节点（multipart）
        std::vector<std::unique_ptr<MimePart>> children;

        // 工具方法
        bool IsMultipart() const {
            return ToLower(contentType).find("multipart") != std::string::npos;
        }

        bool IsTextPlain() const {
            if (contentType.empty()) return true;
            return ToLower(contentType).find("text/plain") != std::string::npos;
        }

        bool IsTextHtml() const {
            return ToLower(contentType).find("text/html") != std::string::npos;
        }
    };

    struct MimeMessage {
        std::unique_ptr<MimePart> root;
        std::string subject;
        std::string displayName;
        std::string from;
        std::string preview;
    };

    class MailParser {
    public:
        static bool Parse(const LikesProgram::Net::Buffer& mailData, MimeMessage& out);
    private:
        // 解析单个节点
        static std::unique_ptr<MimePart> ParsePart(std::string_view data);
        // 解析 Headers
        static void ParseHeaders(std::string_view headers, MimePart& part);

        // Body 解码
        static void DecodeBody(MimePart& part);

        // 解析 Multipart
        static void ParseMultipart(MimePart& part);
    };
}
