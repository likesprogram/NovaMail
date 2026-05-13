#include <LikeHub/DkimVerifier.hpp>
#include <LikesProgram/net/Buffer.hpp>
#include <netdb.h>
#include <algorithm>
#include <arpa/nameser.h>
#include <cctype>
#include <chrono>
#include <cstring>
#include <map>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <resolv.h>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace LikeHub {
    namespace {
        struct HeaderField {
            std::string name;
            std::string raw;   // 不包含末尾 CRLF，保留折叠行
        };

        struct ParsedMail {
            std::vector<HeaderField> headers;
            std::string body;
        };

        struct DkimSignature {
            size_t headerIndex = 0;
            HeaderField header;
            std::unordered_map<std::string, std::string> tags;
        };

        static char LowerAscii(char c) {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        static std::string ToLower(std::string_view input) {
            std::string out;
            out.reserve(input.size());

            for (char c : input) {
                out.push_back(LowerAscii(c));
            }

            return out;
        }

        static bool EqualsIgnoreCase(std::string_view a, std::string_view b) {
            if (a.size() != b.size()) {
                return false;
            }

            for (size_t i = 0; i < a.size(); ++i) {
                if (LowerAscii(a[i]) != LowerAscii(b[i])) {
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

        static bool IsWsp(char c) {
            return c == ' ' || c == '\t';
        }

        static std::string RemoveBase64Whitespace(std::string_view input) {
            std::string out;
            out.reserve(input.size());

            for (unsigned char c : input) {
                if (!std::isspace(c)) {
                    out.push_back(static_cast<char>(c));
                }
            }

            return out;
        }

        static std::string Base64Encode(const unsigned char* data, size_t len) {
            BIO* b64 = BIO_new(BIO_f_base64());
            BIO* mem = BIO_new(BIO_s_mem());

            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
            b64 = BIO_push(b64, mem);

            BIO_write(b64, data, static_cast<int>(len));
            BIO_flush(b64);

            BUF_MEM* buffer = nullptr;
            BIO_get_mem_ptr(b64, &buffer);

            std::string out(buffer->data, buffer->length);
            BIO_free_all(b64);

            return out;
        }

        static std::string Base64Encode(const std::string& data) {
            return Base64Encode(
                reinterpret_cast<const unsigned char*>(data.data()),
                data.size()
            );
        }

        static std::string Base64Decode(std::string_view input) {
            std::string clean = RemoveBase64Whitespace(input);

            BIO* b64 = BIO_new(BIO_f_base64());
            BIO* mem = BIO_new_mem_buf(clean.data(), static_cast<int>(clean.size()));

            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
            mem = BIO_push(b64, mem);

            std::string out;
            out.resize(clean.size());

            int n = BIO_read(mem, out.data(), static_cast<int>(out.size()));
            if (n < 0) {
                BIO_free_all(mem);
                return {};
            }

            out.resize(static_cast<size_t>(n));
            BIO_free_all(mem);

            return out;
        }

        static std::string Sha256Base64(std::string_view data) {
            unsigned char md[EVP_MAX_MD_SIZE];
            unsigned int mdLen = 0;

            EVP_MD_CTX* ctx = EVP_MD_CTX_new();
            EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
            EVP_DigestUpdate(ctx, data.data(), data.size());
            EVP_DigestFinal_ex(ctx, md, &mdLen);
            EVP_MD_CTX_free(ctx);

            return Base64Encode(md, mdLen);
        }

        static ParsedMail ParseMail(std::string_view rawMail) {
            ParsedMail mail;

            size_t split = rawMail.find("\r\n\r\n");
            size_t sepLen = 4;

            if (split == std::string_view::npos) {
                split = rawMail.find("\n\n");
                sepLen = 2;
            }

            std::string_view headerBlock;
            if (split == std::string_view::npos) {
                headerBlock = rawMail;
                mail.body.clear();
            } else {
                headerBlock = rawMail.substr(0, split);
                mail.body = std::string(rawMail.substr(split + sepLen));
            }

            std::vector<std::string> lines;
            size_t pos = 0;

            while (pos < headerBlock.size()) {
                size_t lf = headerBlock.find('\n', pos);

                std::string line;
                if (lf == std::string_view::npos) {
                    line = std::string(headerBlock.substr(pos));
                    pos = headerBlock.size();
                } else {
                    line = std::string(headerBlock.substr(pos, lf - pos));
                    pos = lf + 1;
                }

                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                lines.push_back(std::move(line));
            }

            HeaderField current;
            bool hasCurrent = false;

            for (const auto& line : lines) {
                if (!line.empty() && (line[0] == ' ' || line[0] == '\t')) {
                    if (hasCurrent) {
                        current.raw += "\r\n";
                        current.raw += line;
                    }
                    continue;
                }

                if (hasCurrent) {
                    mail.headers.push_back(std::move(current));
                    current = HeaderField{};
                }

                size_t colon = line.find(':');
                if (colon == std::string::npos) {
                    continue;
                }

                current.name = std::string(line.substr(0, colon));
                current.raw = line;
                hasCurrent = true;
            }

            if (hasCurrent) {
                mail.headers.push_back(std::move(current));
            }

            return mail;
        }

        static std::string UnfoldHeaderValue(std::string_view rawHeader) {
            size_t colon = rawHeader.find(':');
            if (colon == std::string_view::npos) {
                return {};
            }

            std::string_view value = rawHeader.substr(colon + 1);
            std::string out;
            out.reserve(value.size());

            for (size_t i = 0; i < value.size(); ++i) {
                if (value[i] == '\r' &&
                    i + 2 < value.size() &&
                    value[i + 1] == '\n' &&
                    IsWsp(value[i + 2])) {
                    out.push_back(' ');
                    i += 2;

                    while (i + 1 < value.size() && IsWsp(value[i + 1])) {
                        ++i;
                    }
                } else if (value[i] == '\n' &&
                           i + 1 < value.size() &&
                           IsWsp(value[i + 1])) {
                    out.push_back(' ');
                    i += 1;

                    while (i + 1 < value.size() && IsWsp(value[i + 1])) {
                        ++i;
                    }
                } else {
                    out.push_back(value[i]);
                }
            }

            return out;
        }

        static std::unordered_map<std::string, std::string> ParseTagList(std::string_view value) {
            std::unordered_map<std::string, std::string> tags;

            size_t pos = 0;
            while (pos < value.size()) {
                size_t semi = value.find(';', pos);
                if (semi == std::string_view::npos) {
                    semi = value.size();
                }

                std::string_view item = value.substr(pos, semi - pos);
                size_t eq = item.find('=');

                if (eq != std::string_view::npos) {
                    std::string key = ToLower(Trim(item.substr(0, eq)));
                    std::string val = Trim(item.substr(eq + 1));

                    if (!key.empty() && tags.find(key) == tags.end()) {
                        tags.emplace(std::move(key), std::move(val));
                    }
                }

                pos = semi + 1;
            }

            return tags;
        }

        static std::vector<std::string> SplitColonList(std::string_view input) {
            std::vector<std::string> out;

            size_t pos = 0;
            while (pos <= input.size()) {
                size_t colon = input.find(':', pos);
                if (colon == std::string_view::npos) {
                    colon = input.size();
                }

                std::string item = Trim(input.substr(pos, colon - pos));
                if (!item.empty()) {
                    out.push_back(item);
                }

                if (colon == input.size()) {
                    break;
                }

                pos = colon + 1;
            }

            return out;
        }

        static std::pair<DkimCanonicalization, DkimCanonicalization> ParseCanonicalization(const std::string& cTag) {
            if (cTag.empty()) {
                return {DkimCanonicalization::Simple, DkimCanonicalization::Simple};
            }

            std::string c = ToLower(Trim(cTag));

            size_t slash = c.find('/');
            std::string h = slash == std::string::npos ? c : c.substr(0, slash);
            std::string b = slash == std::string::npos ? "simple" : c.substr(slash + 1);

            auto parseOne = [](const std::string& v) {
                if (v == "relaxed") {
                    return DkimCanonicalization::Relaxed;
                }
                return DkimCanonicalization::Simple;
            };

            return {parseOne(h), parseOne(b)};
        }

        static std::string CanonName(DkimCanonicalization c) {
            return c == DkimCanonicalization::Relaxed ? "relaxed" : "simple";
        }

        static std::string CanonPair(DkimCanonicalization h, DkimCanonicalization b) {
            return CanonName(h) + "/" + CanonName(b);
        }

        static std::string CanonicalizeHeaderSimple(const HeaderField& h) {
            return h.raw + "\r\n";
        }

        static std::string CanonicalizeHeaderRelaxed(const HeaderField& h) {
            size_t colon = h.raw.find(':');
            if (colon == std::string::npos) {
                return {};
            }

            std::string name = ToLower(Trim(h.raw.substr(0, colon)));
            std::string value = UnfoldHeaderValue(h.raw);

            std::string compact;
            compact.reserve(value.size());

            bool lastWsp = false;
            for (char c : value) {
                if (IsWsp(c) || c == '\r' || c == '\n') {
                    if (!lastWsp) {
                        compact.push_back(' ');
                        lastWsp = true;
                    }
                } else {
                    compact.push_back(c);
                    lastWsp = false;
                }
            }

            compact = Trim(compact);

            return name + ":" + compact + "\r\n";
        }

        static std::string CanonicalizeHeader(const HeaderField& h, DkimCanonicalization canon) {
            if (canon == DkimCanonicalization::Relaxed) {
                return CanonicalizeHeaderRelaxed(h);
            }

            return CanonicalizeHeaderSimple(h);
        }

        static std::vector<std::string> SplitBodyLines(std::string_view body) {
            std::vector<std::string> lines;

            size_t pos = 0;
            while (pos < body.size()) {
                size_t lf = body.find('\n', pos);

                std::string line;
                if (lf == std::string_view::npos) {
                    line = std::string(body.substr(pos));
                    pos = body.size();
                } else {
                    line = std::string(body.substr(pos, lf - pos));
                    pos = lf + 1;
                }

                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                lines.push_back(std::move(line));
            }

            return lines;
        }

        static std::string CanonicalizeBodySimple(std::string_view body) {
            std::vector<std::string> lines = SplitBodyLines(body);

            while (!lines.empty() && lines.back().empty()) {
                lines.pop_back();
            }

            if (lines.empty()) {
                return "\r\n";
            }

            std::string out;
            for (const auto& line : lines) {
                out += line;
                out += "\r\n";
            }

            return out;
        }

        static std::string CanonicalizeBodyRelaxed(std::string_view body) {
            std::vector<std::string> lines = SplitBodyLines(body);

            for (auto& line : lines) {
                while (!line.empty() && IsWsp(line.back())) {
                    line.pop_back();
                }

                std::string compact;
                compact.reserve(line.size());

                bool lastWsp = false;
                for (char c : line) {
                    if (IsWsp(c)) {
                        if (!lastWsp) {
                            compact.push_back(' ');
                            lastWsp = true;
                        }
                    } else {
                        compact.push_back(c);
                        lastWsp = false;
                    }
                }

                line.swap(compact);
            }

            while (!lines.empty() && lines.back().empty()) {
                lines.pop_back();
            }

            if (lines.empty()) {
                return "";
            }

            std::string out;
            for (const auto& line : lines) {
                out += line;
                out += "\r\n";
            }

            return out;
        }

        static std::string CanonicalizeBody(std::string_view body, DkimCanonicalization canon) {
            if (canon == DkimCanonicalization::Relaxed) {
                return CanonicalizeBodyRelaxed(body);
            }

            return CanonicalizeBodySimple(body);
        }

        static std::string EmptyDkimBValue(std::string rawHeader) {
            size_t colon = rawHeader.find(':');
            if (colon == std::string::npos) {
                return rawHeader;
            }

            size_t pos = colon + 1;

            while (pos < rawHeader.size()) {
                while (pos < rawHeader.size() &&
                       (std::isspace(static_cast<unsigned char>(rawHeader[pos])) ||
                        rawHeader[pos] == ';')) {
                    ++pos;
                }

                size_t keyStart = pos;
                while (pos < rawHeader.size() &&
                       (std::isalnum(static_cast<unsigned char>(rawHeader[pos])) ||
                        rawHeader[pos] == '_')) {
                    ++pos;
                }

                std::string key = ToLower(rawHeader.substr(keyStart, pos - keyStart));

                while (pos < rawHeader.size() && std::isspace(static_cast<unsigned char>(rawHeader[pos]))) {
                    ++pos;
                }

                if (pos >= rawHeader.size() || rawHeader[pos] != '=') {
                    size_t semi = rawHeader.find(';', pos);
                    if (semi == std::string::npos) {
                        break;
                    }
                    pos = semi + 1;
                    continue;
                }

                ++pos;

                size_t valueStart = pos;
                size_t semi = rawHeader.find(';', valueStart);
                size_t valueEnd = semi == std::string::npos ? rawHeader.size() : semi;

                if (key == "b") {
                    rawHeader.erase(valueStart, valueEnd - valueStart);
                    return rawHeader;
                }

                if (semi == std::string::npos) {
                    break;
                }

                pos = semi + 1;
            }

            return rawHeader;
        }

        static std::vector<DkimSignature> ExtractDkimSignatures(const ParsedMail& mail) {
            std::vector<DkimSignature> signatures;

            for (size_t i = 0; i < mail.headers.size(); ++i) {
                if (!EqualsIgnoreCase(mail.headers[i].name, "DKIM-Signature")) {
                    continue;
                }

                std::string value = UnfoldHeaderValue(mail.headers[i].raw);

                DkimSignature sig;
                sig.headerIndex = i;
                sig.header = mail.headers[i];
                sig.tags = ParseTagList(value);

                signatures.push_back(std::move(sig));
            }

            return signatures;
        }

        static bool HeaderListContainsFrom(const std::vector<std::string>& names) {
            for (const auto& name : names) {
                if (EqualsIgnoreCase(name, "From")) {
                    return true;
                }
            }

            return false;
        }

        static std::string BuildSignedHeaderData(const ParsedMail& mail, const std::vector<std::string>& hList, size_t dkimHeaderIndex, const HeaderField& dkimHeaderWithEmptyB, DkimCanonicalization headerCanon) {
            std::string data;
            std::vector<bool> used(mail.headers.size(), false);

            for (const auto& wanted : hList) {
                for (size_t i = mail.headers.size(); i > 0; --i) {
                    size_t idx = i - 1;

                    if (idx == dkimHeaderIndex) {
                        continue;
                    }

                    if (used[idx]) {
                        continue;
                    }

                    if (!EqualsIgnoreCase(mail.headers[idx].name, wanted)) {
                        continue;
                    }

                    data += CanonicalizeHeader(mail.headers[idx], headerCanon);
                    used[idx] = true;
                    break;
                }
            }

            data += CanonicalizeHeader(dkimHeaderWithEmptyB, headerCanon);
            return data;
        }

        static std::vector<std::string> QueryTxtRecords(const std::string& name, bool& tempFail) {
            tempFail = false;

            unsigned char answer[NS_PACKETSZ * 4];

            int len = res_search(
                name.c_str(),
                ns_c_in,
                ns_t_txt,
                answer,
                sizeof(answer)
            );

            if (len < 0) {
                if (h_errno == TRY_AGAIN) {
                    tempFail = true;
                }
                return {};
            }

            ns_msg handle;
            if (ns_initparse(answer, len, &handle) < 0) {
                tempFail = true;
                return {};
            }

            std::vector<std::string> records;

            int count = ns_msg_count(handle, ns_s_an);
            for (int i = 0; i < count; ++i) {
                ns_rr rr;
                if (ns_parserr(&handle, ns_s_an, i, &rr) != 0) {
                    continue;
                }

                if (ns_rr_type(rr) != ns_t_txt) {
                    continue;
                }

                const unsigned char* rdata = ns_rr_rdata(rr);
                int rdlen = ns_rr_rdlen(rr);

                std::string txt;
                int pos = 0;

                while (pos < rdlen) {
                    unsigned char chunkLen = rdata[pos++];
                    if (pos + chunkLen > rdlen) {
                        break;
                    }

                    txt.append(
                        reinterpret_cast<const char*>(rdata + pos),
                        chunkLen
                    );

                    pos += chunkLen;
                }

                if (!txt.empty()) {
                    records.push_back(std::move(txt));
                }
            }

            return records;
        }

        static std::unordered_map<std::string, std::string> LookupDkimKeyRecord(const std::string& domain, const std::string& selector, bool& tempFail) {
            std::string dnsName = selector + "._domainkey." + domain;

            auto records = QueryTxtRecords(dnsName, tempFail);
            if (tempFail || records.empty()) {
                return {};
            }

            for (const auto& record : records) {
                auto tags = ParseTagList(record);

                auto p = tags.find("p");
                if (p == tags.end()) {
                    continue;
                }

                auto v = tags.find("v");
                if (v != tags.end() && !EqualsIgnoreCase(Trim(v->second), "DKIM1")) {
                    continue;
                }

                return tags;
            }

            return {};
        }

        static EVP_PKEY* LoadPublicKeyFromDkimP(const std::string& pValue) {
            std::string der = Base64Decode(pValue);
            if (der.empty()) {
                return nullptr;
            }

            const unsigned char* ptr = reinterpret_cast<const unsigned char*>(der.data());

            // 主流 DKIM DNS p= 通常是 SubjectPublicKeyInfo DER 的 Base64。
            EVP_PKEY* pkey = d2i_PUBKEY(nullptr, &ptr, static_cast<long>(der.size()));
            return pkey;
        }

        static EVP_PKEY* LoadPrivateKeyFromPem(const std::string& pem) {
            BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
            if (!bio) {
                return nullptr;
            }

            EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
            BIO_free(bio);

            return pkey;
        }

        static bool VerifyRsaSha256(EVP_PKEY* publicKey, std::string_view data, const std::string& signatureBytes) {
            EVP_MD_CTX* ctx = EVP_MD_CTX_new();
            if (!ctx) {
                return false;
            }

            bool ok = false;

            if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, publicKey) == 1 &&
                EVP_DigestVerifyUpdate(ctx, data.data(), data.size()) == 1) {
                int rc = EVP_DigestVerifyFinal(
                    ctx,
                    reinterpret_cast<const unsigned char*>(signatureBytes.data()),
                    signatureBytes.size()
                );

                ok = rc == 1;
            }

            EVP_MD_CTX_free(ctx);
            return ok;
        }

        static std::string SignRsaSha256(EVP_PKEY* privateKey, std::string_view data) {
            EVP_MD_CTX* ctx = EVP_MD_CTX_new();
            if (!ctx) {
                throw std::runtime_error("EVP_MD_CTX_new failed");
            }

            if (EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, privateKey) != 1) {
                EVP_MD_CTX_free(ctx);
                throw std::runtime_error("EVP_DigestSignInit failed");
            }

            if (EVP_DigestSignUpdate(ctx, data.data(), data.size()) != 1) {
                EVP_MD_CTX_free(ctx);
                throw std::runtime_error("EVP_DigestSignUpdate failed");
            }

            size_t sigLen = 0;
            if (EVP_DigestSignFinal(ctx, nullptr, &sigLen) != 1) {
                EVP_MD_CTX_free(ctx);
                throw std::runtime_error("EVP_DigestSignFinal size failed");
            }

            std::string signature;
            signature.resize(sigLen);

            if (EVP_DigestSignFinal(
                    ctx,
                    reinterpret_cast<unsigned char*>(signature.data()),
                    &sigLen
                ) != 1) {
                EVP_MD_CTX_free(ctx);
                throw std::runtime_error("EVP_DigestSignFinal failed");
            }

            signature.resize(sigLen);
            EVP_MD_CTX_free(ctx);

            return signature;
        }

        static bool IsHashAllowedByKeyRecord(const std::unordered_map<std::string, std::string>& keyTags, const std::string& algorithm) {
            auto h = keyTags.find("h");
            if (h == keyTags.end()) {
                return true;
            }

            std::string hashName;
            if (algorithm == "rsa-sha256") {
                hashName = "sha256";
            } else {
                return false;
            }

            auto allowed = SplitColonList(h->second);
            for (const auto& item : allowed) {
                if (EqualsIgnoreCase(Trim(item), hashName)) {
                    return true;
                }
            }

            return false;
        }

        static std::string JoinHeaderListForDkim(const std::vector<std::string>& names) {
            std::string out;

            for (size_t i = 0; i < names.size(); ++i) {
                if (i != 0) {
                    out += ":";
                }

                out += ToLower(names[i]);
            }

            return out;
        }

        static std::vector<std::string> ExistingHeadersToSign(const ParsedMail& mail, const std::vector<std::string>& desired) {
            std::vector<std::string> out;

            for (const auto& name : desired) {
                bool exists = false;

                for (const auto& h : mail.headers) {
                    if (EqualsIgnoreCase(h.name, name)) {
                        exists = true;
                        break;
                    }
                }

                if (exists) {
                    out.push_back(name);
                }
            }

            bool hasFrom = false;
            for (const auto& h : out) {
                if (EqualsIgnoreCase(h, "From")) {
                    hasFrom = true;
                    break;
                }
            }

            if (!hasFrom) {
                throw std::runtime_error("DKIM signing requires From header");
            }

            return out;
        }

        static std::string BuildDkimHeaderWithoutSignature(const DkimSignOptions& opt, const std::string& hTag, const std::string& bodyHash, size_t bodyLength) {
            using namespace std::chrono;

            auto now = duration_cast<seconds>(
                system_clock::now().time_since_epoch()
            ).count();

            std::string header;

            header += "DKIM-Signature: ";
            header += "v=1; ";
            header += "a=rsa-sha256; ";
            header += "c=" + CanonPair(opt.headerCanon, opt.bodyCanon) + "; ";
            header += "d=" + opt.domain + "; ";
            header += "s=" + opt.selector + "; ";

            if (!opt.identity.empty()) {
                header += "i=" + opt.identity + "; ";
            }

            header += "t=" + std::to_string(now) + "; ";

            if (opt.includeBodyLength) {
                header += "l=" + std::to_string(bodyLength) + "; ";
            }

            header += "h=" + hTag + "; ";
            header += "bh=" + bodyHash + "; ";
            header += "b=";

            return header;
        }

        static std::string FoldDkimHeaderRelaxedSafe(const std::string& header) {
            constexpr size_t LIMIT = 76;

            if (header.size() <= LIMIT) {
                return header;
            }

            std::string out;
            size_t pos = 0;

            while (pos < header.size()) {
                size_t remaining = header.size() - pos;
                size_t take = std::min(LIMIT, remaining);

                if (pos == 0) {
                    out.append(header, pos, take);
                } else {
                    out += "\r\n\t";
                    out.append(header, pos, take);
                }

                pos += take;
            }

            return out;
        }

    }

    DkimVerifyResult DkimVerifier::Verify(const LikesProgram::Net::Buffer& rawMail) {
        std::string_view raw(
            reinterpret_cast<const char*>(rawMail.Peek()),
            rawMail.ReadableBytes()
        );

        return Verify(raw);
    }

    DkimVerifyResult DkimVerifier::Verify(std::string_view rawMail) {
        ParsedMail mail = ParseMail(rawMail);
        auto signatures = ExtractDkimSignatures(mail);

        if (signatures.empty()) {
            DkimVerifyResult result;
            result.status = DkimVerifyStatus::None;
            result.reason = "DKIM-Signature header not found";
            return result;
        }

        DkimVerifyResult lastFailure;
        lastFailure.status = DkimVerifyStatus::Fail;
        lastFailure.reason = "No DKIM signature passed";

        for (const auto& sig : signatures) {
            DkimVerifyResult result;

            auto getTag = [&](const std::string& key) -> std::string {
                auto it = sig.tags.find(key);
                if (it == sig.tags.end()) {
                    return {};
                }
                return Trim(it->second);
            };

            std::string v = getTag("v");
            std::string a = ToLower(getTag("a"));
            std::string d = getTag("d");
            std::string s = getTag("s");
            std::string h = getTag("h");
            std::string bh = RemoveBase64Whitespace(getTag("bh"));
            std::string b = RemoveBase64Whitespace(getTag("b"));
            std::string c = getTag("c");
            std::string iTag = getTag("i");
            std::string lTag = getTag("l");

            result.domain = d;
            result.selector = s;
            result.identity = iTag;
            result.algorithm = a;
            result.canonicalization = c.empty() ? "simple/simple" : c;

            if (!v.empty() && !EqualsIgnoreCase(v, "1")) {
                result.status = DkimVerifyStatus::PermFail;
                result.reason = "Unsupported DKIM version";
                lastFailure = result;
                continue;
            }

            if (a != "rsa-sha256") {
                result.status = DkimVerifyStatus::PermFail;
                result.reason = "Unsupported or insecure DKIM algorithm: " + a;
                lastFailure = result;
                continue;
            }

            if (d.empty() || s.empty() || h.empty() || bh.empty() || b.empty()) {
                result.status = DkimVerifyStatus::PermFail;
                result.reason = "Required DKIM tag missing";
                lastFailure = result;
                continue;
            }

            std::vector<std::string> hList = SplitColonList(h);
            if (hList.empty() || !HeaderListContainsFrom(hList)) {
                result.status = DkimVerifyStatus::PermFail;
                result.reason = "DKIM h= must include From";
                lastFailure = result;
                continue;
            }

            auto [headerCanon, bodyCanon] = ParseCanonicalization(c);

            std::string canonBody = CanonicalizeBody(mail.body, bodyCanon);

            if (!lTag.empty()) {
                try {
                    size_t bodyLen = static_cast<size_t>(std::stoull(lTag));
                    if (bodyLen < canonBody.size()) {
                        canonBody.resize(bodyLen);
                    }
                } catch (...) {
                    result.status = DkimVerifyStatus::PermFail;
                    result.reason = "Invalid DKIM l= body length";
                    lastFailure = result;
                    continue;
                }
            }

            std::string computedBh = Sha256Base64(canonBody);
            if (computedBh != bh) {
                result.status = DkimVerifyStatus::Fail;
                result.reason = "DKIM body hash mismatch";
                lastFailure = result;
                continue;
            }

            bool tempFail = false;
            auto keyTags = LookupDkimKeyRecord(d, s, tempFail);

            if (tempFail) {
                result.status = DkimVerifyStatus::TempFail;
                result.reason = "DKIM DNS temporary failure";
                lastFailure = result;
                continue;
            }

            if (keyTags.empty()) {
                result.status = DkimVerifyStatus::PermFail;
                result.reason = "DKIM DNS key record not found";
                lastFailure = result;
                continue;
            }

            auto pIt = keyTags.find("p");
            if (pIt == keyTags.end() || Trim(pIt->second).empty()) {
                result.status = DkimVerifyStatus::PermFail;
                result.reason = "DKIM key revoked or p= missing";
                lastFailure = result;
                continue;
            }

            auto kIt = keyTags.find("k");
            if (kIt != keyTags.end() && !EqualsIgnoreCase(Trim(kIt->second), "rsa")) {
                result.status = DkimVerifyStatus::PermFail;
                result.reason = "Unsupported DKIM key type";
                lastFailure = result;
                continue;
            }

            if (!IsHashAllowedByKeyRecord(keyTags, a)) {
                result.status = DkimVerifyStatus::PermFail;
                result.reason = "DKIM key record does not allow sha256";
                lastFailure = result;
                continue;
            }

            EVP_PKEY* publicKey = LoadPublicKeyFromDkimP(pIt->second);
            if (!publicKey) {
                result.status = DkimVerifyStatus::PermFail;
                result.reason = "Invalid DKIM public key";
                lastFailure = result;
                continue;
            }

            if (EVP_PKEY_bits(publicKey) < 1024) {
                EVP_PKEY_free(publicKey);

                result.status = DkimVerifyStatus::PermFail;
                result.reason = "DKIM RSA key is shorter than 1024 bits";
                lastFailure = result;
                continue;
            }

            HeaderField dkimEmpty;
            dkimEmpty.name = sig.header.name;
            dkimEmpty.raw = EmptyDkimBValue(sig.header.raw);

            std::string signedData = BuildSignedHeaderData(
                mail,
                hList,
                sig.headerIndex,
                dkimEmpty,
                headerCanon
            );

            std::string signatureBytes = Base64Decode(b);

            bool ok = VerifyRsaSha256(publicKey, signedData, signatureBytes);
            EVP_PKEY_free(publicKey);

            if (!ok) {
                result.status = DkimVerifyStatus::Fail;
                result.reason = "DKIM RSA signature verify failed";
                lastFailure = result;
                continue;
            }

            result.status = DkimVerifyStatus::Pass;
            result.reason = "DKIM verification passed";
            return result;
        }

        return lastFailure;
    }

    std::string DkimVerifier::Sign(std::string_view rawMail, const DkimSignOptions& options) {
        if (options.domain.empty()) {
            throw std::invalid_argument("DKIM domain is empty");
        }

        if (options.selector.empty()) {
            throw std::invalid_argument("DKIM selector is empty");
        }

        if (options.privateKeyPem.empty()) {
            throw std::invalid_argument("DKIM private key PEM is empty");
        }

        ParsedMail mail = ParseMail(rawMail);

        std::vector<std::string> headersToSign =
            ExistingHeadersToSign(mail, options.headersToSign);

        std::string canonBody = CanonicalizeBody(mail.body, options.bodyCanon);
        std::string bodyHash = Sha256Base64(canonBody);

        std::string hTag = JoinHeaderListForDkim(headersToSign);

        std::string dkimHeaderRaw = BuildDkimHeaderWithoutSignature(
            options,
            hTag,
            bodyHash,
            canonBody.size()
        );

        HeaderField dkimEmpty;
        dkimEmpty.name = "DKIM-Signature";
        dkimEmpty.raw = dkimHeaderRaw;

        std::string signedData = BuildSignedHeaderData(
            mail,
            headersToSign,
            static_cast<size_t>(-1),
            dkimEmpty,
            options.headerCanon
        );

        EVP_PKEY* privateKey = LoadPrivateKeyFromPem(options.privateKeyPem);
        if (!privateKey) {
            throw std::runtime_error("Failed to load DKIM private key");
        }

        if (EVP_PKEY_bits(privateKey) < 1024) {
            EVP_PKEY_free(privateKey);
            throw std::runtime_error("DKIM RSA private key must be at least 1024 bits");
        }

        std::string signatureBytes = SignRsaSha256(privateKey, signedData);
        EVP_PKEY_free(privateKey);

        std::string signatureBase64 = Base64Encode(signatureBytes);

        std::string finalHeader = dkimHeaderRaw + signatureBase64;

        // 默认 relaxed header canonicalization 下，折叠不会影响验签。
        // 如果你强制 simple 签名，建议不要折叠或在签名前就固定折叠格式。
        if (options.headerCanon == DkimCanonicalization::Relaxed) {
            finalHeader = FoldDkimHeaderRelaxedSafe(finalHeader);
        }

        return finalHeader;
    }
}
