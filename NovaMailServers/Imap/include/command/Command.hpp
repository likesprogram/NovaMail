#pragma once
#include <string>
#include <string_view>
#include <cstddef>
#include <array>
#include <memory>
#include <atomic>
#include <LikesProgram/net/Buffer.hpp>

namespace NovaMail {
    namespace Imap {
        enum class CommandVerb : std::size_t {
            Unknown = 0,
            // RFC 9051 6.1 - Any State
            CAPABILITY, NOOP, LOGOUT,
            // RFC 9051 6.2 - Not Authenticated State
            STARTTLS, AUTHENTICATE, LOGIN,
            // RFC 9051 6.3 - Authenticated State
            ENABLE, SELECT, EXAMINE, CREATE,
            DELETE_,       // DELETE 是语义关键字风格，避免直接用 DELETE
            RENAME, SUBSCRIBE, UNSUBSCRIBE, LIST, LSUB, NAMESPACE, STATUS, APPEND, IDLE,
            // RFC 9051 6.4 - Selected State
            CHECK, CLOSE, UNSELECT, EXPUNGE, SEARCH, FETCH, STORE, COPY, MOVE, UID,
            // 扩展命令入口：RFC 9051 6.5 Experimental/Expansion
            // 具体扩展命令不建议直接塞进基础枚举，后续按 capability 扩展。
            Extension,
            COUNT
        };
        enum class UidSubCommandVerb {
            Unknown,
            FETCH,
            STORE,
            SEARCH,
            COPY,
            MOVE
        };
        enum class InputMode {
            CommandLine,     // 普通命令模式：<tag> <command> <arguments>
            ActiveLine,      // 多阶段行模式：AUTHENTICATE 后续 base64、IDLE 的 DONE
            LiteralBytes     // 原始字节模式：APPEND literal
        };
        class Command {
        public:
            Command() = default;
            // 若有一整行，会自动消费一行
            Command(LikesProgram::Net::Buffer& in);

            std::string_view RawLine() const noexcept { return m_rawLine; }
            std::string_view  Tag() const noexcept {return m_tag; }
            CommandVerb Verb() const noexcept { return m_verb; }
            bool UidMode() const noexcept { return m_uidMode; }
            UidSubCommandVerb UidSubCommand() const noexcept { return m_uidSubVerb; }
            std::string_view  Arguments() const noexcept { return m_arguments; }
            bool NoComplete() const noexcept { return !m_complete; }
        private:
            std::string m_rawLine;
            std::string m_tag;
            CommandVerb m_verb = CommandVerb::Unknown;
            bool m_uidMode = false;
            UidSubCommandVerb m_uidSubVerb = UidSubCommandVerb::Unknown;
            std::string m_arguments;
            bool m_complete = false;
        };
    }
}