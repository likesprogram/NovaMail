#include <command/AppendCommand.hpp>
#include <Session.hpp>
#include <command/Command.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <algorithm>
#include <cctype>
#include <string_view>

namespace NovaMail {
    namespace Imap {
        static void ConsumeOptionalCrlf(LikesProgram::Net::Buffer& in) {
            if (in.ReadableBytes() >= 2) {
                const char* p = reinterpret_cast<const char*>(in.Peek());
            
                if (p[0] == '\r' && p[1] == '\n') {
                    in.Consume(2);
                    return;
                }
            }
        
            if (in.ReadableBytes() >= 1) {
                const char* p = reinterpret_cast<const char*>(in.Peek());
            
                if (p[0] == '\n') {
                    in.Consume(1);
                    return;
                }
            }
        }

        void AppendCommand::Handle(
            const std::shared_ptr<Session>& session,
            const Command& command
        ) {
            if (!session) return;

            ResetState();

            m_tag = std::string(command.Tag());

            const auto* user = session->GetUser();
            if (!user) {
                session->ReplyTagged(m_tag, Status::No, "Authentication required");
                ResetState();
                return;
            }

            if (!ParseAppendArguments(command.Arguments(), m_args)) {
                session->ReplyTagged(m_tag, Status::Bad, "Bad APPEND arguments");
                ResetState();
                return;
            }

            if (!BeginAppendLiteral(session)) {
                ResetState();
                return;
            }
        }

        InputResult AppendCommand::InputData(
            const std::shared_ptr<Session>& session,
            LikesProgram::Net::Buffer& in
        ) {
            if (!session) return InputResult::Failed;

            switch (m_stage) {
            case Stage::ReadingLiteral:
                return ReadAppendLiteral(session, in);

            case Stage::None:
            default:
                return InputResult::Failed;
            }
        }

        bool AppendCommand::ParseAppendArguments(
            std::string_view args,
            AppendArguments& out
        ) const {
            out = AppendArguments{};

            size_t pos = 0;

            if (!ReadAString(args, pos, out.mailboxName)) {
                return false;
            }

            SkipSpaces(args, pos);

            if (pos >= args.size()) {
                return false;
            }

            /*
             * APPEND mailbox [flag-list] [date-time] literal
             *
             * flag-list:
             *   (\Seen \Draft)
             */
            if (args[pos] == '(') {
                if (!ReadFlagList(args, pos, out.flags)) {
                    return false;
                }

                SkipSpaces(args, pos);
            }

            /*
             * date-time:
             *   "10-May-2026 20:10:00 +0000"
             */
            if (pos < args.size() && args[pos] == '"') {
                std::string dateTime;

                if (!ReadQuotedString(args, pos, dateTime)) {
                    return false;
                }

                out.internalDate = std::move(dateTime);

                SkipSpaces(args, pos);
            }

            if (!ReadLiteralSpec(args, pos, out.literal)) {
                return false;
            }

            SkipSpaces(args, pos);

            return pos == args.size();
        }

        bool AppendCommand::ReadAString(
            std::string_view text,
            size_t& pos,
            std::string& out
        ) const {
            out.clear();

            SkipSpaces(text, pos);

            if (pos >= text.size()) {
                return false;
            }

            if (text[pos] == '"') {
                return ReadQuotedString(text, pos, out);
            }

            /*
             * 当前先支持 atom mailbox：
             *   Sent
             *   INBOX
             *   Drafts
             *
             * 如果后续要支持 mailbox literal：
             *   APPEND {4}
             *   Sent ...
             * 需要额外增加 mailbox literal 状态机。
             */
            const size_t begin = pos;

            while (pos < text.size()) {
                const char ch = text[pos];

                if (ch == ' ' || ch == '\t' || ch == '(' || ch == '{') {
                    break;
                }

                const auto uch = static_cast<unsigned char>(ch);

                if (uch < 0x20 || uch == 0x7F) {
                    return false;
                }

                if (ch == '"' || ch == '\\') {
                    return false;
                }

                ++pos;
            }

            if (pos == begin) {
                return false;
            }

            out = std::string(text.substr(begin, pos - begin));
            return true;
        }

        bool AppendCommand::ReadQuotedString(
            std::string_view text,
            size_t& pos,
            std::string& out
        ) const {
            out.clear();

            SkipSpaces(text, pos);

            if (pos >= text.size() || text[pos] != '"') {
                return false;
            }

            ++pos;

            while (pos < text.size()) {
                const char ch = text[pos++];

                if (ch == '"') {
                    return true;
                }

                if (ch == '\\') {
                    if (pos >= text.size()) {
                        return false;
                    }

                    const char escaped = text[pos++];

                    if (escaped == '\r' || escaped == '\n') {
                        return false;
                    }

                    out.push_back(escaped);
                    continue;
                }

                const auto uch = static_cast<unsigned char>(ch);

                if (uch < 0x20 || uch == 0x7F) {
                    return false;
                }

                out.push_back(ch);
            }

            return false;
        }

        bool AppendCommand::ReadFlagList(
            std::string_view text,
            size_t& pos,
            std::vector<std::string>& flags
        ) const {
            flags.clear();

            SkipSpaces(text, pos);

            if (pos >= text.size() || text[pos] != '(') {
                return false;
            }

            ++pos;

            for (;;) {
                SkipSpaces(text, pos);

                if (pos >= text.size()) {
                    return false;
                }

                if (text[pos] == ')') {
                    ++pos;
                    return true;
                }

                const size_t begin = pos;

                while (pos < text.size()) {
                    const char ch = text[pos];

                    if (ch == ' ' || ch == '\t' || ch == ')') {
                        break;
                    }

                    const auto uch = static_cast<unsigned char>(ch);

                    if (uch < 0x20 || uch == 0x7F) {
                        return false;
                    }

                    ++pos;
                }

                if (pos == begin) {
                    return false;
                }

                flags.emplace_back(text.substr(begin, pos - begin));
            }
        }

        bool AppendCommand::ReadLiteralSpec(
            std::string_view text,
            size_t& pos,
            LiteralSpec& literal
        ) const {
            literal = LiteralSpec{};

            SkipSpaces(text, pos);

            if (pos >= text.size() || text[pos] != '{') {
                return false;
            }

            ++pos;

            const size_t numberBegin = pos;

            while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) {
                ++pos;
            }

            if (pos == numberBegin) {
                return false;
            }

            try {
                literal.size = static_cast<size_t>(
                    std::stoull(std::string(text.substr(numberBegin, pos - numberBegin)))
                );
            } catch (...) {
                return false;
            }

            if (pos < text.size() && text[pos] == '+') {
                literal.nonSync = true;
                ++pos;
            }

            if (pos >= text.size() || text[pos] != '}') {
                return false;
            }

            ++pos;

            return true;
        }

        void AppendCommand::SkipSpaces(
            std::string_view text,
            size_t& pos
        ) const noexcept {
            while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t')) {
                ++pos;
            }
        }

        bool AppendCommand::BeginAppendLiteral(
            const std::shared_ptr<Session>& session
        ) {
            if (!session) {
                return false;
            }

            if (m_args.literal.size > kMaxAppendLiteralBytes) {
                session->ReplyTagged(m_tag, Status::No, "APPEND literal too large");
                return false;
            }

            m_stage = Stage::ReadingLiteral;
            m_expectedLiteralBytes = m_args.literal.size;
            m_receivedLiteralBytes = 0;
            m_literalBuffer.clear();
            m_literalBuffer.reserve(m_expectedLiteralBytes);

            /*
             * LITERAL+:
             *   APPEND "Sent" (\Seen) {338+}
             *
             * 客户端不等 continuation，已经直接发送 literal。
             * 所以 nonSync=true 时绝对不能回复 '+ Ready...'。
             */
            if (!m_args.literal.nonSync) {
                session->ReplyContinuation("Ready for literal data");
            }

            session->BeginLiteralBytes(CommandVerb::APPEND, m_tag);

            LogInfo(
                u"IMAP APPEND literal begin: tag={} mailbox={} size={} literalPlus={}",
                m_tag,
                m_args.mailboxName,
                static_cast<int64_t>(m_expectedLiteralBytes),
                m_args.literal.nonSync ? "true" : "false"
            );

            return true;
        }

        InputResult AppendCommand::ReadAppendLiteral(
            const std::shared_ptr<Session>& session,
            LikesProgram::Net::Buffer& in
        ) {
            if (!session) {
                return InputResult::Failed;
            }

            while (m_receivedLiteralBytes < m_expectedLiteralBytes) {
                const size_t readable = in.ReadableBytes();

                if (readable == 0) {
                    return InputResult::NeedMoreData;
                }

                const size_t remaining = m_expectedLiteralBytes - m_receivedLiteralBytes;
                const size_t take = std::min(readable, remaining);

                m_literalBuffer.append(
                    reinterpret_cast<const char*>(in.Peek()),
                    take
                );

                in.Consume(take);
                m_receivedLiteralBytes += take;
            }

            ExecuteAppend(session);

            // 某些测试工具或客户端可能在 APPEND literal 后额外发送 CRLF。
            // APPEND 的 literal 已经按 size 精确读取完成，这里的 CRLF 不是邮件内容。
            ConsumeOptionalCrlf(in);

            ResetState();

            /*
             * 返回 Continue 更稳：
             * 如果 LITERAL+ 情况下，客户端把后续命令也和 literal 一起发过来了，
             * 外层 Session 可以继续处理 buffer 中剩余命令。
             */
            return InputResult::Continue;
        }

        void AppendCommand::ExecuteAppend(
            const std::shared_ptr<Session>& session
        ) {
            if (!session) {
                return;
            }

            const auto* user = session->GetUser();

            if (!user) {
                session->ReplyTagged(m_tag, Status::No, "Authentication required");
                return;
            }

            /*
             * 你的系统当前 SMTP 发信成功后已经自动保存 Sent。
             * 因此 Thunderbird 的 APPEND "Sent" 只用于客户端保存已发送副本。
             *
             * 当前策略：
             *   - Sent：完整消费 literal，但不重复入库，直接 OK
             *   - Drafts / 其他文件夹：这里预留真实保存入口
             */
            if (IsSentMailbox(m_args.mailboxName)) {
                LogInfo(
                    u"IMAP APPEND Sent ignored after literal consumed: userId={} mailbox={} size={}",
                    static_cast<int64_t>(user->userId),
                    m_args.mailboxName,
                    static_cast<int64_t>(m_literalBuffer.size())
                );

                session->ReplyTagged(m_tag, Status::Ok, "APPEND completed");
                return;
            }

            /*
             * 如果你暂时还没做 APPEND 入库，其他文件夹也必须先消费 literal 后再回复。
             * 这样不会污染后续 IMAP 命令流。
             *
             * 后续建议：
             *   Drafts：保存草稿
             *   其他文件夹：作为邮件导入处理
             */
            LogWarn(
                u"IMAP APPEND target mailbox not persisted yet: userId={} mailbox={} size={}",
                static_cast<int64_t>(user->userId),
                m_args.mailboxName,
                static_cast<int64_t>(m_literalBuffer.size())
            );

            session->ReplyTagged(m_tag, Status::No, "APPEND storage not supported for this mailbox");
        }

        bool AppendCommand::IsSentMailbox(std::string_view mailboxName) const {
            const std::string name = ToUpperAscii(TrimCopy(mailboxName));

            return name == "SENT" ||
                   name == "SENT ITEMS" ||
                   name == "SENT MESSAGES" ||
                   name == "已发送" ||
                   name == "已发送邮件";
        }

        bool AppendCommand::IsDraftsMailbox(std::string_view mailboxName) const {
            const std::string name = ToUpperAscii(TrimCopy(mailboxName));

            return name == "DRAFTS" ||
                   name == "DRAFT" ||
                   name == "草稿" ||
                   name == "草稿箱";
        }

        std::string AppendCommand::ToUpperAscii(std::string_view value) {
            std::string out;
            out.reserve(value.size());

            for (char ch : value) {
                out.push_back(static_cast<char>(
                    std::toupper(static_cast<unsigned char>(ch))
                ));
            }

            return out;
        }

        std::string AppendCommand::TrimCopy(std::string_view value) {
            while (!value.empty() &&
                   (value.front() == ' ' || value.front() == '\t' ||
                    value.front() == '\r' || value.front() == '\n')) {
                value.remove_prefix(1);
            }

            while (!value.empty() &&
                   (value.back() == ' ' || value.back() == '\t' ||
                    value.back() == '\r' || value.back() == '\n')) {
                value.remove_suffix(1);
            }

            return std::string(value);
        }

        void AppendCommand::ResetState() {
            m_stage = Stage::None;

            m_tag.clear();
            m_args = AppendArguments{};

            m_expectedLiteralBytes = 0;
            m_receivedLiteralBytes = 0;
            m_literalBuffer.clear();
        }

    }
}