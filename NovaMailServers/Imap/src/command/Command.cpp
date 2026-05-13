#include <command/Command.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <algorithm>
#include <cctype>

namespace NovaMail {
    namespace Imap {
        static bool IsSpace(char ch) noexcept {
            return ch == ' ' || ch == '\t';
        }

        static std::string ToUpperAscii(std::string_view text) {
            std::string out;
            out.reserve(text.size());

            for (char ch : text) {
                out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            }

            return out;
        }

        static void TrimLeft(std::string_view& text) noexcept {
            while (!text.empty() && IsSpace(text.front())) text.remove_prefix(1);
        }

        static void TrimRight(std::string_view& text) noexcept {
            while (!text.empty() && IsSpace(text.back())) text.remove_suffix(1);
        }

        static std::string TrimCopy(std::string_view text) {
            TrimLeft(text);
            TrimRight(text);
            return std::string(text);
        }

        static bool ReadAtom(std::string_view& text, std::string& out) {
            out.clear();

            TrimLeft(text);

            while (!text.empty()) {
                char ch = text.front();

                if (IsSpace(ch)) break;

                /*
                 * IMAP command line 中基础 tag / command 不应该包含控制字符。
                 * 更严格的 atom-special 校验可以后续放到 ArgumentScanner 里。
                 */
                unsigned char uch = static_cast<unsigned char>(ch);
                if (uch < 0x20 || uch == 0x7F) return false;

                out.push_back(ch);
                text.remove_prefix(1);
            }

            return !out.empty();
        }

        static CommandVerb ParseCommandVerb(std::string_view verbText) {
            const std::string verb = ToUpperAscii(verbText);

            if (verb == "CAPABILITY")   return CommandVerb::CAPABILITY;
            if (verb == "NOOP")         return CommandVerb::NOOP;
            if (verb == "LOGOUT")       return CommandVerb::LOGOUT;

            if (verb == "STARTTLS")     return CommandVerb::STARTTLS;
            if (verb == "AUTHENTICATE") return CommandVerb::AUTHENTICATE;
            if (verb == "LOGIN")        return CommandVerb::LOGIN;

            if (verb == "ENABLE")       return CommandVerb::ENABLE;
            if (verb == "SELECT")       return CommandVerb::SELECT;
            if (verb == "EXAMINE")      return CommandVerb::EXAMINE;
            if (verb == "CREATE")       return CommandVerb::CREATE;
            if (verb == "DELETE")       return CommandVerb::DELETE_;
            if (verb == "RENAME")       return CommandVerb::RENAME;
            if (verb == "SUBSCRIBE")    return CommandVerb::SUBSCRIBE;
            if (verb == "UNSUBSCRIBE")  return CommandVerb::UNSUBSCRIBE;
            if (verb == "LIST")         return CommandVerb::LIST;
            if (verb == "LSUB")         return CommandVerb::LSUB;
            if (verb == "NAMESPACE")    return CommandVerb::NAMESPACE;
            if (verb == "STATUS")       return CommandVerb::STATUS;
            if (verb == "APPEND")       return CommandVerb::APPEND;
            if (verb == "IDLE")         return CommandVerb::IDLE;

            if (verb == "CHECK")        return CommandVerb::CHECK;
            if (verb == "CLOSE")        return CommandVerb::CLOSE;
            if (verb == "UNSELECT")     return CommandVerb::UNSELECT;
            if (verb == "EXPUNGE")      return CommandVerb::EXPUNGE;
            if (verb == "SEARCH")       return CommandVerb::SEARCH;
            if (verb == "FETCH")        return CommandVerb::FETCH;
            if (verb == "STORE")        return CommandVerb::STORE;
            if (verb == "COPY")         return CommandVerb::COPY;
            if (verb == "MOVE")         return CommandVerb::MOVE;
            if (verb == "UID")          return CommandVerb::UID;

            return CommandVerb::Unknown;
        }

        static UidSubCommandVerb ParseUidSubCommandVerb(std::string_view verbText) {
            const std::string verb = ToUpperAscii(verbText);

            if (verb == "FETCH")  return UidSubCommandVerb::FETCH;
            if (verb == "STORE")  return UidSubCommandVerb::STORE;
            if (verb == "SEARCH") return UidSubCommandVerb::SEARCH;
            if (verb == "COPY")   return UidSubCommandVerb::COPY;
            if (verb == "MOVE")   return UidSubCommandVerb::MOVE;

            return UidSubCommandVerb::Unknown;
        }

        static CommandVerb ToCommandVerb(UidSubCommandVerb verb) noexcept {
            switch (verb) {
                case UidSubCommandVerb::FETCH:
                    return CommandVerb::FETCH;
                case UidSubCommandVerb::STORE:
                    return CommandVerb::STORE;
                case UidSubCommandVerb::SEARCH:
                    return CommandVerb::SEARCH;
                case UidSubCommandVerb::COPY:
                    return CommandVerb::COPY;
                case UidSubCommandVerb::MOVE:
                    return CommandVerb::MOVE;
                default:
                    return CommandVerb::Unknown;
            }
        }

        Command::Command(LikesProgram::Net::Buffer& in) {
            // 在 buffer 中查找 CRLF
            const char* data = reinterpret_cast<const char*>(in.Peek());
            size_t len = in.ReadableBytes();

            // 查找行尾
            const char* crlf = nullptr;
            for (size_t i = 0; i + 1 < len; ++i) {
                if (data[i] == '\r' && data[i + 1] == '\n') {
                    crlf = data + i; break;
                }
            }

            // 没有完整一行，等待更多数据
            if (!crlf) return;

            // 得到完整的一行
            m_complete = true;

            // 构造一行（不包含 CRLF）
            size_t lineLen = static_cast<size_t>(crlf - data);
            m_rawLine = std::string(data, lineLen);

            // 消费这一行（+2 是 CRLF）
            in.Consume(lineLen + 2);

            std::string_view view(m_rawLine);

            std::string tag;
            std::string verbText;

            if (!ReadAtom(view, tag)) {
                m_verb = CommandVerb::Unknown;
                return;
            }

            if (!ReadAtom(view, verbText)) {
                m_tag = std::move(tag);
                m_verb = CommandVerb::Unknown;
                return;
            }

            m_tag = std::move(tag);

            const CommandVerb parsedVerb = ParseCommandVerb(verbText);

            if (parsedVerb == CommandVerb::UID) {
                std::string subVerbText;
                if (!ReadAtom(view, subVerbText)) {
                    m_uidMode = true;
                    m_uidSubVerb = UidSubCommandVerb::Unknown;
                    m_verb = CommandVerb::Unknown;
                    m_arguments = TrimCopy(view);
                    return;
                }

                m_uidMode = true;
                m_uidSubVerb = ParseUidSubCommandVerb(subVerbText);
                m_verb = ToCommandVerb(m_uidSubVerb);
                m_arguments = TrimCopy(view);
                return;
            }

            m_verb = parsedVerb;
            m_arguments = TrimCopy(view);
        }
    }
}