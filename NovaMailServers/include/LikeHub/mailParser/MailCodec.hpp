#pragma once
#include <string>
#include <string_view>

namespace LikeHub {
    class MailCodec {
    public:
        // charset提取
        static std::string ExtractCharset(const std::string& contentType);

        // 转码核心
        static std::string ToUTF8(const std::string& input, const std::string& charset);

        // Transfer-Encoding
        static std::string DecodeTransfer(std::string_view body, const std::string& encoding);

        // Base64
        static std::string Base64Decode(std::string_view input);

        // QP
        static std::string QPDecode(std::string_view in);
    private:
        // 工具
        static void Trim(std::string& s);
        static void ToUpperInPlace(std::string& s);
        static void ToLowerInPlace(std::string& s);

        // Latin1 fallback
        static std::string Latin1ToUTF8(const std::string& input);
    };
}
