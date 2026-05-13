#include <LikeHub/mailParser/MailCodec.hpp>
#include <algorithm>
#include <iostream>
#include <LikesProgram/String.hpp>

namespace LikeHub {
    static std::string StripBase64(std::string_view input) {
        std::string out;
        out.reserve(input.size());

        for (char c : input) {
            if (std::isalnum((unsigned char)c) || c == '+' || c == '/' || c == '=') {
                out += c;
            }
        }
        return out;
    }

    std::string MailCodec::ExtractCharset(const std::string& contentType) {
        auto pos = contentType.find("charset=");
        if (pos == std::string::npos) return "UTF-8";
    
        std::string charset = contentType.substr(pos + 8);
    
        auto semi = charset.find(';');
        if (semi != std::string::npos)
            charset = charset.substr(0, semi);
    
        Trim(charset);
        ToUpperInPlace(charset);
    
        if (!charset.empty() && charset.front() == '"')
            charset.erase(0, 1);
        if (!charset.empty() && charset.back() == '"')
            charset.pop_back();
    
        return charset;
    }
    
    std::string MailCodec::ToUTF8(const std::string& input, const std::string& charset) {
        using Encoding = LikesProgram::String::Encoding;
    
        try {
            if (charset == "UTF-8" || charset == "UTF8") {
                return input;
            }
        
            Encoding enc = Encoding::UTF8;
        
            if (charset == "GBK" || charset == "GB2312") {
                enc = Encoding::GBK;
            }
            else if (charset == "UTF-16") {
                enc = Encoding::UTF16;
            }
            else if (charset == "UTF-32") {
                enc = Encoding::UTF32;
            }
            else if (charset == "ISO-8859-1" || charset == "WINDOWS-1252") {
                // fallback：当作 latin1 → UTF-8
                return Latin1ToUTF8(input);
            }
        
            LikesProgram::String s(input, enc);
            return s.ToStdString(Encoding::UTF8);
        }
        catch (...) {
            return input; // fallback
        }
    }

    std::string MailCodec::DecodeTransfer(std::string_view body, const std::string& encoding) {
        std::string enc = encoding;
        ToLowerInPlace(enc);
    
        if (enc == "base64") return Base64Decode(StripBase64(body));
        else if (enc == "quoted-printable") return QPDecode(body);
    
        return std::string(body);
    }

    void MailCodec::Trim(std::string& s) {
        while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(0, 1);
        while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
    }

    void MailCodec::ToUpperInPlace(std::string& s) {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    }

    void MailCodec::ToLowerInPlace(std::string& s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    }

    std::string MailCodec::Latin1ToUTF8(const std::string& input) {
        std::string out;
        for (unsigned char c : input) {
            if (c < 0x80) out.push_back(c);
            else {
                out.push_back(0xC0 | (c >> 6));
                out.push_back(0x80 | (c & 0x3F));
            }
        }
        return out;
    }

    std::string MailCodec::Base64Decode(std::string_view input) {
        static const std::string table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
        std::string out;
        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; i++) T[table[i]] = i;
    
        int val = 0, valb = -8;
        for (unsigned char c : input) {
            if (T[c] == -1) continue;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                out.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }

    std::string MailCodec::QPDecode(std::string_view in) {
        std::string out;
        for (size_t i = 0; i < in.size(); ++i) {
            if (in[i] == '=' && i + 2 < in.size()) {
                if (in[i+1] == '\r' && in[i+2] == '\n') {
                    i += 2;
                    continue;
                }
                std::string hex = std::string(in.substr(i+1, 2));
                char c = (char)strtol(hex.c_str(), nullptr, 16);
                out.push_back(c);
                i += 2;
            } else {
                out.push_back(in[i]);
            }
        }
        return out;
    }
}
