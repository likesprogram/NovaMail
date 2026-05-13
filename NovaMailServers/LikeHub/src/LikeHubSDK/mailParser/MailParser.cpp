#include <LikeHub/mailParser/MailParser.hpp>
#include <LikeHub/mailParser/MailCodec.hpp>
#include <algorithm>
#include <cctype>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace LikeHub {
    // 统一换行符
    static std::string NormalizeNewlines(std::string_view input) {
        std::string out;
        out.reserve(input.size() + input.size() / 10); // 预留一点扩展空间

        for (size_t i = 0; i < input.size(); ++i) {
            char c = input[i];

            if (c == '\r') {
                // 已经是 CRLF
                if (i + 1 < input.size() && input[i + 1] == '\n') {
                    out += "\r\n";
                    ++i;
                } else {
                    // 单独 CR → 转 CRLF
                    out += "\r\n";
                }
            }
            else if (c == '\n') {
                // 单独 LF → 转 CRLF
                out += "\r\n";
            }
            else {
                out += c;
            }
        }

        return out;
    }

    static inline std::string Trim(std::string_view v) {
        size_t start = 0;
        while (start < v.size() && std::isspace((unsigned char)v[start])) start++;

        size_t end = v.size();
        while (end > start && std::isspace((unsigned char)v[end - 1])) end--;

        return std::string(v.substr(start, end - start));
    }

    static inline char ToLowerAscii(char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    static bool StartsWithCaseInsensitive(std::string_view s, size_t pos, std::string_view prefix) {
        if (pos + prefix.size() > s.size()) {
            return false;
        }

        for (size_t i = 0; i < prefix.size(); ++i) {
            if (ToLowerAscii(s[pos + i]) != ToLowerAscii(prefix[i])) {
                return false;
            }
        }

        return true;
    }

    static size_t FindCaseInsensitive(std::string_view s, std::string_view needle, size_t pos = 0) {
        if (needle.empty()) {
            return pos <= s.size() ? pos : std::string_view::npos;
        }

        if (needle.size() > s.size()) {
            return std::string_view::npos;
        }

        for (size_t i = pos; i + needle.size() <= s.size(); ++i) {
            if (StartsWithCaseInsensitive(s, i, needle)) {
                return i;
            }
        }

        return std::string_view::npos;
    }

    static void ReplaceAll(std::string& s, std::string_view from, std::string_view to) {
        if (from.empty()) {
            return;
        }

        size_t pos = 0;
        while ((pos = s.find(from.data(), pos, from.size())) != std::string::npos) {
            s.replace(pos, from.size(), to.data(), to.size());
            pos += to.size();
        }
    }

    static std::string CollapseWhitespace(std::string_view input) {
        std::string out;
        out.reserve(input.size());

        bool lastSpace = false;

        for (unsigned char c : input) {
            if (std::isspace(c)) {
                if (!lastSpace) {
                    out += ' ';
                    lastSpace = true;
                }
            } else {
                out += static_cast<char>(c);
                lastSpace = false;
            }
        }

        return Trim(out);
    }

    static bool IsBlockTagName(std::string_view tagName) {
        static constexpr std::string_view tags[] = {
            "br", "p", "div", "section", "article", "header", "footer",
            "table", "thead", "tbody", "tfoot", "tr", "td", "th",
            "ul", "ol", "li",
            "h1", "h2", "h3", "h4", "h5", "h6",
            "blockquote", "pre"
        };

        for (auto tag : tags) {
            if (tagName.size() == tag.size()) {
                bool same = true;
                for (size_t i = 0; i < tag.size(); ++i) {
                    if (ToLowerAscii(tagName[i]) != tag[i]) {
                        same = false;
                        break;
                    }
                }

                if (same) {
                    return true;
                }
            }
        }

        return false;
    }

    static std::string ExtractTagName(std::string_view tagContent) {
        size_t pos = 0;

        while (pos < tagContent.size() && std::isspace(static_cast<unsigned char>(tagContent[pos]))) {
            ++pos;
        }

        if (pos < tagContent.size() && tagContent[pos] == '/') {
            ++pos;
        }

        while (pos < tagContent.size() && std::isspace(static_cast<unsigned char>(tagContent[pos]))) {
            ++pos;
        }

        size_t start = pos;

        while (pos < tagContent.size()) {
            unsigned char c = static_cast<unsigned char>(tagContent[pos]);
            if (!std::isalnum(c)) {
                break;
            }
            ++pos;
        }

        return std::string(tagContent.substr(start, pos - start));
    }

    static void RemoveHtmlBlock(std::string& html, std::string_view tagName) {
        std::string openTag = "<";
        openTag += std::string(tagName);

        std::string closeTag = "</";
        closeTag += std::string(tagName);

        size_t searchPos = 0;

        while (true) {
            size_t start = FindCaseInsensitive(html, openTag, searchPos);
            if (start == std::string::npos) {
                break;
            }

            size_t openEnd = html.find('>', start);
            if (openEnd == std::string::npos) {
                html.erase(start);
                break;
            }

            size_t closeStart = FindCaseInsensitive(html, closeTag, openEnd + 1);
            if (closeStart == std::string::npos) {
                html.erase(start, openEnd - start + 1);
                searchPos = start;
                continue;
            }

            size_t closeEnd = html.find('>', closeStart);
            if (closeEnd == std::string::npos) {
                html.erase(start);
                break;
            }

            html.erase(start, closeEnd - start + 1);
            searchPos = start;
        }
    }

    static void RemoveHtmlComments(std::string& html) {
        size_t pos = 0;

        while (true) {
            size_t start = html.find("<!--", pos);
            if (start == std::string::npos) {
                break;
            }

            size_t end = html.find("-->", start + 4);
            if (end == std::string::npos) {
                html.erase(start);
                break;
            }

            html.erase(start, end - start + 3);
            pos = start;
        }
    }

    static std::string DecodeHtmlEntities(std::string text) {
        ReplaceAll(text, "&nbsp;", " ");
        ReplaceAll(text, "&amp;", "&");
        ReplaceAll(text, "&lt;", "<");
        ReplaceAll(text, "&gt;", ">");
        ReplaceAll(text, "&quot;", "\"");
        ReplaceAll(text, "&#39;", "'");
        ReplaceAll(text, "&apos;", "'");

        return text;
    }

    static std::string HtmlToPlainText(const std::string& html) {
        std::string text = html;

        // 1. 删除 HTML 注释
        RemoveHtmlComments(text);

        // 2. 删除不可见块内容。这里必须整块删除，否则 CSS/JS 会残留到 preview。
        RemoveHtmlBlock(text, "style");
        RemoveHtmlBlock(text, "script");
        RemoveHtmlBlock(text, "head");
        RemoveHtmlBlock(text, "noscript");
        RemoveHtmlBlock(text, "svg");
        RemoveHtmlBlock(text, "xml");

        // 3. 删除标签，块级标签转换为空格，避免文字粘连。
        std::string out;
        out.reserve(text.size());

        size_t pos = 0;

        while (pos < text.size()) {
            size_t lt = text.find('<', pos);
            if (lt == std::string::npos) {
                out.append(text, pos, std::string::npos);
                break;
            }

            out.append(text, pos, lt - pos);

            size_t gt = text.find('>', lt + 1);
            if (gt == std::string::npos) {
                break;
            }

            std::string tagName = ExtractTagName(std::string_view(text).substr(lt + 1, gt - lt - 1));
            if (!tagName.empty() && IsBlockTagName(tagName)) {
                out += ' ';
            }

            pos = gt + 1;
        }

        // 4. HTML 实体解码
        out = DecodeHtmlEntities(std::move(out));

        // 5. 压缩空白
        return CollapseWhitespace(out);
    }

    static std::string NormalizePlainTextPreview(const std::string& plainText) {
        return CollapseWhitespace(plainText);
    }

    // 解析 字符编码
    static std::string DecodeMimeWord(const std::string& input) {
        // =?UTF-8?B?...?=
        if (input.find("=?") == std::string::npos) return input;

        std::string out;
        size_t pos = 0;

        while (true) {
            size_t start = input.find("=?", pos);
            if (start == std::string::npos) {
                out += input.substr(pos);
                break;
            }

            out += input.substr(pos, start - pos);

            size_t end = input.find("?=", start);
            if (end == std::string::npos) break;

            auto token = input.substr(start + 2, end - start - 2);

            auto q1 = token.find('?');
            auto q2 = token.find('?', q1 + 1);

            if (q1 == std::string::npos || q2 == std::string::npos) break;

            std::string charset = token.substr(0, q1);
            std::string encoding = token.substr(q1 + 1, 1);
            std::string data = token.substr(q2 + 1);

            if (encoding == "B" || encoding == "b") {
                out += MailCodec::Base64Decode(data);
            } else if (encoding == "Q" || encoding == "q") {
                std::string tmp = data;
                std::replace(tmp.begin(), tmp.end(), '_', ' ');
                out += MailCodec::QPDecode(tmp);
            }

            pos = end + 2;
        }

        return out;
    }

    // 提取信息
    static void ExtractBasicFields(const std::unordered_map<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEq>& headers, MimeMessage& out) {
        // Subject
        if (auto it = headers.find("Subject"); it != headers.end()) out.subject = DecodeMimeWord(it->second);

        // From
        if (auto it = headers.find("From"); it != headers.end()) {
            std::string raw = DecodeMimeWord(it->second);

            out.from = raw;

            std::string name;
            std::string email;

            // 提取 email
            size_t lt = raw.find('<');
            size_t gt = raw.find('>');

            if (lt != std::string::npos && gt != std::string::npos && gt > lt) {
                email = raw.substr(lt + 1, gt - lt - 1);
                name = raw.substr(0, lt);
            } else {
                // 可能只有邮箱
                email = raw;
            }

            // 去 comment
            auto removeComments = [](std::string& s) {
                std::string out;
                int depth = 0;

                for (char c : s) {
                    if (c == '(') depth++;
                    else if (c == ')') depth--;
                    else if (depth == 0) out += c;
                }

                s.swap(out);
            };

            removeComments(name);

            // trim
            name = Trim(name);

            // 去引号
            if (!name.empty() && name.front() == '"' && name.back() == '"') name = name.substr(1, name.size() - 2);

            // fallback
            if (name.empty()) name = email;

            out.displayName = name;
        }

        // Preview
        if (out.root) {
            std::string preview;
            bool previewFromHtml = false;

            // 从 text/plain 优先获取
            std::function<void(const MimePart*)> dfsPlain = [&](const MimePart* part) {
                if (!part || !preview.empty()) return;
                if (part->IsTextPlain() && !part->decodedBody.empty()) {
                    preview = part->decodedBody;
                    previewFromHtml = false;
                    return;
                }

                for (const auto& child : part->children) dfsPlain(child.get());
            };

            dfsPlain(out.root.get());

            // fallback → html
            if (preview.empty()) {
                std::function<void(const MimePart*)> dfsHtml = [&](const MimePart* part) {
                    if (!part || !preview.empty()) return;

                    if (part->IsTextHtml() && !part->decodedBody.empty()) {
                        preview = part->decodedBody;
                        previewFromHtml = true;
                        return;
                    }

                    for (const auto& child : part->children) dfsHtml(child.get());
                };

                dfsHtml(out.root.get());
            }

            // 清洗 preview
            if (!preview.empty()) {
                std::string normalized;
                if (previewFromHtml) normalized = HtmlToPlainText(preview);
                else normalized = NormalizePlainTextPreview(preview);
                out.preview = normalized;
            }
        }
    }

    // 分离 Header 与 Body
    static bool SplitHeaderBody(std::string_view raw, std::string_view& headers, std::string_view& body) {
        size_t pos = raw.find("\r\n\r\n");
        if (pos == std::string_view::npos) return false;

        headers = raw.substr(0, pos);
        body = raw.substr(pos + 4);
        return true;
    }

    // Boundary 提取
    static std::string ExtractBoundary(const std::string& contentType) {
        auto lower = ToLower(contentType);

        size_t pos = lower.find("boundary=");
        if (pos == std::string::npos) return "";

        pos += 9;

        if (pos >= contentType.size()) return "";

        // 支持 quoted boundary
        if (contentType[pos] == '"') {
            size_t end = contentType.find('"', pos + 1);
            if (end == std::string::npos) return "";
            return contentType.substr(pos + 1, end - pos - 1);
        }

        // 非 quoted
        size_t end = contentType.find(';', pos);
        if (end == std::string::npos) end = contentType.size();

        return Trim(contentType.substr(pos, end - pos));
    }

    bool MailParser::Parse(const LikesProgram::Net::Buffer& mailData, MimeMessage& out) {
        std::string_view rawStr(reinterpret_cast<const char*>(mailData.Peek()), mailData.ReadableBytes());
        std::string normalized = NormalizeNewlines(rawStr); // 统一换行符

        std::string_view raw(normalized);
        auto root = ParsePart(raw);
        if (!root) return false;
        out.root = std::move(root);

        // 提取 envelope 信息（顶层 header）
        ExtractBasicFields(out.root->headers, out);

        return true;
    }

    std::unique_ptr<MimePart> MailParser::ParsePart(std::string_view raw) {
        std::string_view headers, body;

        if (!SplitHeaderBody(raw, headers, body)) return nullptr;

        auto part = std::make_unique<MimePart>();

        ParseHeaders(headers, *part);

        part->rawBody = std::string(body);

        DecodeBody(*part);

        if (part->IsMultipart()) ParseMultipart(*part);

        return part;
    }

    void MailParser::ParseHeaders(std::string_view headers, MimePart& part) {
        std::string currentKey;
        std::string currentValue;

        size_t pos = 0;

        while (pos < headers.size()) {
            size_t end = headers.find("\r\n", pos);

            std::string_view line;
            if (end == std::string_view::npos) {
                // 处理最后一行
                line = headers.substr(pos);
                pos = headers.size();
            } else {
                line = headers.substr(pos, end - pos);
                pos = end + 2;
            }

            // header folding
            if (!line.empty() && (line[0] == ' ' || line[0] == '\t')) {
                currentValue += " " + Trim(line);
            } else {
                if (!currentKey.empty()) part.headers[currentKey] = currentValue;

                auto colon = line.find(':');
                if (colon != std::string_view::npos) {
                    currentKey = Trim(line.substr(0, colon));
                    currentValue = Trim(line.substr(colon + 1));
                    while (!currentValue.empty() && (currentValue.back() == '\r' || currentValue.back() == '\n')) currentValue.pop_back();
                } else {
                    currentKey.clear();
                    currentValue.clear();
                }
            }
        }

        if (!currentKey.empty()) part.headers[currentKey] = currentValue;

        // 提取关键字段
        if (auto it = part.headers.find("Content-Type"); it != part.headers.end()) {
            part.contentType = it->second;
            part.charset = MailCodec::ExtractCharset(it->second);
        }
        if (auto it2 = part.headers.find("Content-Transfer-Encoding"); it2 != part.headers.end()) {
            part.transferEncoding = ToLower(Trim(it2->second));
        }
    }

    void MailParser::DecodeBody(MimePart& part) {
        if (part.transferEncoding.empty()) {
            part.decodedBody = part.rawBody;
        } else {
            part.decodedBody = MailCodec::DecodeTransfer(
                part.rawBody,
                part.transferEncoding
            );
        }

        part.decodedBody = MailCodec::ToUTF8(
            part.decodedBody,
            part.charset
        );
    }

    void MailParser::ParseMultipart(MimePart& part) {
        std::string boundary = ExtractBoundary(part.contentType);
        if (boundary.empty()) return;

        std::string delim = "--" + boundary;
        std::string_view body = part.rawBody;

        size_t pos = 0;

        while (true) {
            // 必须匹配 "\r\n--boundary" 或开头 "--boundary"
            size_t start = body.find(delim, pos);
            if (start == std::string_view::npos) break;

            // 跳过 boundary
            start += delim.size();

            // end boundary
            if (body.substr(start, 2) == "--") break;

            // 跳过 CRLF
            if (body.substr(start, 2) == "\r\n") start += 2;

            // 下一个 boundary
            size_t next = body.find("\r\n" + delim, start);
            if (next == std::string_view::npos) {
                next = body.find(delim, start); // fallback
                if (next == std::string_view::npos) break;
            }

            std::string_view sub = body.substr(start, next - start);

            auto child = ParsePart(sub);
            if (child) part.children.push_back(std::move(child));

            pos = next;
        }
    }
}
