#include <command/ListCommand.hpp>
#include <Session.hpp>
#include <command/Command.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <algorithm>
#include <cctype>
#include <string_view>

namespace NovaMail {
    namespace Imap {
        static const char* ResponseAtom(Mode mode) {
            return mode == Mode::Lsub ? "LSUB" : "LIST";
        }

        static CommandVerb LiteralVerb(Mode mode) {
            return mode == Mode::Lsub ? CommandVerb::LSUB : CommandVerb::LIST;
        }

        void ListCommand::Handle(const std::shared_ptr<Session> &session, const Command &command) {
            if (!session) return;
            ResetState();

            m_tag = std::string(command.Tag());
            const std::string args = std::string(command.Arguments());

            LiteralSpec literal;
            const auto result = ParseInitialArguments(args, m_args, literal);

            if (result == ParseResult::Bad) {
                session->ReplyTagged(m_tag, Status::Bad, "Bad " + std::string(ResponseAtom(m_mode)) +" arguments");
                ResetState();
                return;
            }

            if (result == ParseResult::NeedReferenceLiteral) {
                if (!BeginLiteralRead(session, m_tag, Stage::ReadingReferenceLiteral, literal)) ResetState();
                return;
            }

            if (result == ParseResult::NeedPatternLiteral) {
                if (!BeginLiteralRead(session, m_tag, Stage::ReadingPatternLiteral, literal)) ResetState();
                return;
            }

            ExecuteList(session, m_tag);
            ResetState();
        }

        InputResult ListCommand::InputData(const std::shared_ptr<Session> &session, LikesProgram::Net::Buffer &in) {
            if (!session) return InputResult::Failed;

            for (;;) {
                switch (m_stage) {
                case Stage::ReadingReferenceLiteral:
                case Stage::ReadingPatternLiteral: {
                    auto result = ReadLiteralBytes(session, in);
                    if (result == InputResult::NeedMoreData || result == InputResult::Failed || result == InputResult::Finished) return result;
                    continue;
                }
                case Stage::NeedPatternLine:
                    return ParsePatternLineFromBuffer(session, in);
                case Stage::NeedCommandEnd:
                    return ConsumeCommandEndAndExecuteList(session, in);
                case Stage::None:
                default: return InputResult::Failed;
                }
            }
        }

        ListCommand::ParseResult ListCommand::ParseInitialArguments(const std::string &args, ListArguments &out, LiteralSpec &literal) const {
            size_t pos = 0;

            if (!ParseOptionalSelectionOptions(args, pos, out)) return ParseResult::Bad;

            std::string referenceName;
            LiteralSpec referenceLiteral;

            if (!ReadAString(args, pos, referenceName, &referenceLiteral)) return ParseResult::Bad;

            if (referenceLiteral.size != 0 || referenceLiteral.nonSync) {
                SkipSpaces(args, pos);

                // literal marker 必须在当前行末尾
                if (pos != args.size()) return ParseResult::Bad;

                literal = referenceLiteral;
                return ParseResult::NeedReferenceLiteral;
            }

            out.referenceName = std::move(referenceName);

            bool needPatternLiteral = false;
            LiteralSpec patternLiteral;

            if (!ReadMailboxPatterns(args, pos, out.mailboxPatterns, patternLiteral, needPatternLiteral))  return ParseResult::Bad;

            if (needPatternLiteral) {
                SkipSpaces(args, pos);

                // pattern literal 也必须位于当前行末尾
                if (pos != args.size()) return ParseResult::Bad;

                literal = patternLiteral;
                return ParseResult::NeedPatternLiteral;
            }

            if (!ParseOptionalReturnOptions(args, pos, out)) return ParseResult::Bad;

            SkipSpaces(args, pos);
            if (pos != args.size()) return ParseResult::Bad;

            if (out.mailboxPatterns.empty()) return ParseResult::Bad;

            return ParseResult::Ok;
        }

        ListCommand::ParseResult ListCommand::ParsePatternLine(const std::string &line, LiteralSpec &literal) {
            size_t pos = 0;

            bool needPatternLiteral = false;
            LiteralSpec patternLiteral;

            if (!ReadMailboxPatterns(line, pos, m_args.mailboxPatterns, patternLiteral, needPatternLiteral)) return ParseResult::Bad;

            if (needPatternLiteral) {
                SkipSpaces(line, pos);

                if (pos != line.size()) return ParseResult::Bad;

                literal = patternLiteral;
                return ParseResult::NeedPatternLiteral;
            }

            if (!ParseOptionalReturnOptions(line, pos, m_args)) return ParseResult::Bad;

            SkipSpaces(line, pos);
            if (pos != line.size()) return ParseResult::Bad;

            if (m_args.mailboxPatterns.empty()) return ParseResult::Bad;

            return ParseResult::Ok;
        }

        bool ListCommand::ParseLiteralSpec(const std::string &text, size_t &pos, LiteralSpec &literal) const {
            literal = LiteralSpec{};

            SkipSpaces(text, pos);

            if (pos >= text.size() || text[pos] != '{') return false;

            ++pos;
            const size_t numberBegin = pos;

            while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])))  ++pos;
            if (pos == numberBegin) return false;

            const size_t numberEnd = pos;

            if (pos < text.size() && text[pos] == '+') {
                literal.nonSync = true;
                ++pos;
            }

            if (pos >= text.size() || text[pos] != '}') return false;

            ++pos;

            try {
                literal.size = static_cast<size_t>(std::stoull(text.substr(numberBegin, numberEnd - numberBegin)));
            } catch (...) {
                return false;
            }

            return true;
        }

        bool ListCommand::ReadAString(const std::string &text, size_t &pos, std::string &out, LiteralSpec *literal) const {
            out.clear();

            if (literal) *literal = LiteralSpec{};

            SkipSpaces(text, pos);

            if (pos >= text.size()) return false;

            if (text[pos] == '{') {
                if (!literal) return false;

                LiteralSpec parsedLiteral;
                if (!ParseLiteralSpec(text, pos, parsedLiteral)) return false;

                *literal = parsedLiteral;
                return true;
            }

            if (text[pos] == '"') {
                ++pos;

                while (pos < text.size()) {
                    const char ch = text[pos++];
                    if (ch == '"') return true;

                    if (ch == '\\') {
                        if (pos >= text.size()) return false;
                        const char escaped = text[pos++];
                        if (escaped == '\r' || escaped == '\n') return false;

                        out.push_back(escaped);
                        continue;
                    }

                    const unsigned char uch = static_cast<unsigned char>(ch);

                    if (uch == '\r' || uch == '\n') return false;
                    out.push_back(ch);
                }
                return false;
            }

            // atom
            while (pos < text.size()) {
                const char ch = text[pos];

                if (ch == ' ' || ch == '\t' || ch == ')') break;

                const unsigned char uch = static_cast<unsigned char>(ch);

                if (uch < 0x20 || uch == 0x7F) return false;

                if (ch == '(' || ch == '{' || ch == '"' || ch == '\\') return false;

                out.push_back(ch);
                ++pos;
            }

            return !out.empty();
        }

        bool ListCommand::ReadMailboxPatterns(const std::string &text, size_t &pos, std::vector<std::string> &patterns, LiteralSpec &literal, bool &needLiteral) const {
            patterns.clear();
            literal = LiteralSpec{};
            needLiteral = false;

            SkipSpaces(text, pos);

            if (pos >= text.size())
            {
                return false;
            }

            // IMAP4rev2 支持 mailbox pattern list，例如:
            // LIST "" ("INBOX" "Sent")
            if (text[pos] == '(')
            {
                ++pos;

                for (;;)
                {
                    SkipSpaces(text, pos);

                    if (pos >= text.size())
                    {
                        return false;
                    }

                    if (text[pos] == ')')
                    {
                        ++pos;
                        break;
                    }

                    std::string item;
                    LiteralSpec itemLiteral;

                    if (!ReadAString(text, pos, item, &itemLiteral))
                    {
                        return false;
                    }

                    // 第一版不支持括号列表内部 literal。
                    // 因为 literal 会打断当前括号结构，状态机需要保存 list continuation。
                    if (itemLiteral.size != 0 || itemLiteral.nonSync)
                    {
                        return false;
                    }

                    patterns.emplace_back(std::move(item));
                }

                return !patterns.empty();
            }

            std::string pattern;
            LiteralSpec patternLiteral;

            if (!ReadAString(text, pos, pattern, &patternLiteral))
            {
                return false;
            }

            if (patternLiteral.size != 0 || patternLiteral.nonSync)
            {
                literal = patternLiteral;
                needLiteral = true;
                return true;
            }

            patterns.emplace_back(std::move(pattern));
            return true;
        }

        bool ListCommand::ParseOptionalSelectionOptions(
            const std::string &text,
            size_t &pos,
            ListArguments &out) const
        {
            SkipSpaces(text, pos);

            if (pos >= text.size() || text[pos] != '(')
            {
                return true;
            }

            std::string options;
            if (!ReadBalancedParentheses(text, pos, options))
            {
                return false;
            }

            if (ContainsWordIgnoreCase(options, "SUBSCRIBED"))
            {
                out.subscribedOnly = true;
            }

            // SPECIAL-USE 作为 selection option 时，这里不做过滤。
            // 真实实现可以只返回带特殊用途的邮箱。
            return true;
        }

        bool ListCommand::ParseOptionalReturnOptions(
            const std::string &text,
            size_t &pos,
            ListArguments &out) const
        {
            SkipSpaces(text, pos);

            if (pos >= text.size())
            {
                return true;
            }

            const size_t save = pos;

            std::string atom;
            LiteralSpec dummy;

            if (!ReadAString(text, pos, atom, &dummy))
            {
                return false;
            }

            if (ToUpperAscii(atom) != "RETURN")
            {
                pos = save;
                return true;
            }

            SkipSpaces(text, pos);

            std::string returnOptions;
            if (!ReadBalancedParentheses(text, pos, returnOptions))
            {
                return false;
            }

            if (ContainsWordIgnoreCase(returnOptions, "SPECIAL-USE"))
            {
                out.returnSpecialUse = true;
            }

            return true;
        }

        bool ListCommand::ReadBalancedParentheses(
            const std::string &text,
            size_t &pos,
            std::string &out) const
        {
            out.clear();

            SkipSpaces(text, pos);

            if (pos >= text.size() || text[pos] != '(')
            {
                return false;
            }

            int depth = 0;
            bool inQuote = false;
            bool escaped = false;

            const size_t begin = pos;

            while (pos < text.size())
            {
                const char ch = text[pos++];

                if (inQuote)
                {
                    if (escaped)
                    {
                        escaped = false;
                        continue;
                    }

                    if (ch == '\\')
                    {
                        escaped = true;
                        continue;
                    }

                    if (ch == '"')
                    {
                        inQuote = false;
                        continue;
                    }

                    if (ch == '\r' || ch == '\n')
                    {
                        return false;
                    }

                    continue;
                }

                if (ch == '"')
                {
                    inQuote = true;
                    continue;
                }

                if (ch == '(')
                {
                    ++depth;
                    continue;
                }

                if (ch == ')')
                {
                    --depth;

                    if (depth == 0)
                    {
                        out = text.substr(begin, pos - begin);
                        return true;
                    }

                    if (depth < 0)
                    {
                        return false;
                    }

                    continue;
                }

                const unsigned char uch = static_cast<unsigned char>(ch);
                if (uch < 0x20 && ch != '\t')
                {
                    return false;
                }
            }

            return false;
        }

        bool ListCommand::BeginLiteralRead(
            const std::shared_ptr<Session> &session,
            const std::string &tag,
            Stage stage,
            const LiteralSpec &literal)
        {
            if (!session)
                return false;

            if (literal.size > kMaxListLiteralBytes)
            {
                session->ReplyTagged(tag, Status::Bad, std::string(ResponseAtom(m_mode)) + " literal too large");
                return false;
            }

            m_stage = stage;
            m_expectedLiteralBytes = literal.size;
            m_receivedLiteralBytes = 0;
            m_literalBuffer.clear();
            m_literalBuffer.reserve(literal.size);

            if (!literal.nonSync)
            {
                session->ReplyContinuation("Ready for literal data");
            }

            session->BeginLiteralBytes(LiteralVerb(m_mode), tag);
            return true;
        }

        InputResult ListCommand::ReadLiteralBytes(
            const std::shared_ptr<Session> &session,
            LikesProgram::Net::Buffer &in)
        {
            if (!session)
                return InputResult::Failed;

            if (m_receivedLiteralBytes < m_expectedLiteralBytes)
            {
                const size_t readable = in.ReadableBytes();

                if (readable == 0)
                {
                    return InputResult::NeedMoreData;
                }

                const size_t remaining = m_expectedLiteralBytes - m_receivedLiteralBytes;
                const size_t take = std::min(readable, remaining);

                m_literalBuffer.append(
                    reinterpret_cast<const char *>(in.Peek()),
                    take);

                in.Consume(take);
                m_receivedLiteralBytes += take;

                if (m_receivedLiteralBytes < m_expectedLiteralBytes)
                {
                    return InputResult::NeedMoreData;
                }
            }

            if (m_stage == Stage::ReadingReferenceLiteral)
            {
                m_args.referenceName = std::move(m_literalBuffer);
                m_literalBuffer.clear();

                m_stage = Stage::NeedPatternLine;
                return InputResult::Continue;
            }

            if (m_stage == Stage::ReadingPatternLiteral)
            {
                m_args.mailboxPatterns.clear();
                m_args.mailboxPatterns.emplace_back(std::move(m_literalBuffer));
                m_literalBuffer.clear();

                m_stage = Stage::NeedCommandEnd;
                return InputResult::Continue;
            }

            return InputResult::Failed;
        }

        InputResult ListCommand::ParsePatternLineFromBuffer(
            const std::shared_ptr<Session> &session,
            LikesProgram::Net::Buffer &in)
        {
            if (!session)
                return InputResult::Failed;

            const size_t readable = in.ReadableBytes();

            if (readable == 0)
            {
                return InputResult::NeedMoreData;
            }

            const auto *data = in.Peek();

            size_t lfPos = static_cast<size_t>(-1);

            for (size_t i = 0; i < readable; ++i)
            {
                if (data[i] == '\n')
                {
                    lfPos = i;
                    break;
                }
            }

            if (lfPos == static_cast<size_t>(-1))
            {
                if (readable > kMaxListLineBytes)
                {
                    session->ReplyTagged(m_tag, Status::Bad, std::string(ResponseAtom(m_mode)) + " continuation line too large");
                    ResetState();
                    return InputResult::Finished;
                }

                return InputResult::NeedMoreData;
            }

            size_t lineLen = lfPos;

            if (lineLen > 0 && data[lineLen - 1] == '\r')
            {
                --lineLen;
            }

            std::string line(reinterpret_cast<const char *>(data), lineLen);
            in.Consume(lfPos + 1);

            LiteralSpec literal;
            const auto result = ParsePatternLine(line, literal);

            if (result == ParseResult::Bad || result == ParseResult::NeedReferenceLiteral)
            {
                session->ReplyTagged(m_tag, Status::Bad, "Bad " + std::string(ResponseAtom(m_mode)) + " mailbox pattern");
                ResetState();
                return InputResult::Finished;
            }

            if (result == ParseResult::NeedPatternLiteral)
            {
                if (!BeginLiteralRead(session, m_tag, Stage::ReadingPatternLiteral, literal))
                {
                    ResetState();
                    return InputResult::Finished;
                }

                return InputResult::Continue;
            }

            ExecuteList(session, m_tag);
            ResetState();
            return InputResult::Finished;
        }

        InputResult ListCommand::ConsumeCommandEndAndExecuteList(
            const std::shared_ptr<Session> &session,
            LikesProgram::Net::Buffer &in)
        {
            if (!session)
                return InputResult::Failed;

            const size_t readable = in.ReadableBytes();

            if (readable == 0)
            {
                return InputResult::NeedMoreData;
            }

            const auto *data = in.Peek();

            size_t pos = 0;

            while (pos < readable && (data[pos] == ' ' || data[pos] == '\t'))
            {
                ++pos;
            }

            if (pos >= readable)
            {
                return InputResult::NeedMoreData;
            }

            if (data[pos] == '\r')
            {
                if (pos + 1 >= readable)
                {
                    return InputResult::NeedMoreData;
                }

                if (data[pos + 1] != '\n')
                {
                    session->ReplyTagged(m_tag, Status::Bad, "Bad " + std::string(ResponseAtom(m_mode)) + " command ending");
                    ResetState();
                    return InputResult::Finished;
                }

                in.Consume(pos + 2);
            }
            else if (data[pos] == '\n')
            {
                in.Consume(pos + 1);
            }
            else
            {
                session->ReplyTagged(m_tag, Status::Bad, "Too many " + std::string(ResponseAtom(m_mode)) + " arguments");
                ResetState();
                return InputResult::Finished;
            }

            ExecuteList(session, m_tag);
            ResetState();
            return InputResult::Finished;
        }

        void ListCommand::ExecuteList(
            const std::shared_ptr<Session> &session,
            const std::string &tag)
        {
            if (!session)
                return;

            const auto *user = session->GetUser();

            if (!user)
            {
                session->ReplyTagged(tag, Status::Bad, "Not authenticated");
                return;
            }

            const uint64_t userId = user->userId;

            auto folders = m_folderService.ListMailboxes(userId);

            // LIST "" ""：查询层级分隔符/root mailbox。
            if (m_args.referenceName.empty() &&
                m_args.mailboxPatterns.size() == 1 &&
                m_args.mailboxPatterns[0].empty())
            {
                std::string rootResponse;
                rootResponse.append(ResponseAtom(m_mode));
                rootResponse.append(" (\\Noselect) \"/\" \"\"");
                session->ReplyUntagged(rootResponse);
                session->ReplyTagged(tag, Status::Ok, std::string(ResponseAtom(m_mode)) + " completed");
                return;
            }

            for (const auto &folder : folders)
            {
                if (!folder.Valid())
                {
                    continue;
                }

                if (m_args.subscribedOnly && !folder.subscribed)
                {
                    continue;
                }

                bool matched = false;

                for (const auto &rawPattern : m_args.mailboxPatterns)
                {
                    const std::string pattern = BuildCanonicalPattern(
                        m_args.referenceName,
                        rawPattern);

                    if (MatchListPattern(folder.imapName, pattern))
                    {
                        matched = true;
                        break;
                    }
                }

                if (!matched)
                {
                    continue;
                }

                std::string response;
                response.reserve(folder.imapName.size() + 128);

                response.append(std::string(ResponseAtom(m_mode)));
                response.push_back(' ');
                response.append(BuildListAttributes(folder));
                response.append(" \"/\" ");
                response.append(QuoteString(folder.imapName));

                session->ReplyUntagged(response);
            }

            LogInfo(
                u"IMAP {} completed: userId={} client={} -> server={} patternCount={}",
                std::string(ResponseAtom(m_mode)),
                static_cast<int64_t>(userId),
                session->GetRemoteAddress().ToString(),
                session->GetLocalAddress().ToString(),
                static_cast<int64_t>(m_args.mailboxPatterns.size()));

            session->ReplyTagged(tag, Status::Ok, std::string(ResponseAtom(m_mode)) + " completed");
        }

        std::string ListCommand::BuildCanonicalPattern(
            const std::string &referenceName,
            const std::string &mailboxPattern) const
        {
            if (referenceName.empty())
            {
                return mailboxPattern;
            }

            if (mailboxPattern.empty())
            {
                return referenceName;
            }

            if (referenceName.back() == kHierarchyDelimiter)
            {
                return referenceName + mailboxPattern;
            }

            return referenceName + kHierarchyDelimiter + mailboxPattern;
        }

        bool ListCommand::MatchListPattern(
            const std::string &mailboxName,
            const std::string &pattern) const
        {
            return MatchListPatternRecursive(mailboxName, 0, pattern, 0);
        }

        bool ListCommand::MatchListPatternRecursive(
            std::string_view mailboxName,
            size_t namePos,
            std::string_view pattern,
            size_t patternPos) const
        {
            if (patternPos == pattern.size())
            {
                return namePos == mailboxName.size();
            }

            const char pc = pattern[patternPos];

            if (pc == '*')
            {
                // * 匹配任意字符，包括层级分隔符
                for (size_t i = namePos; i <= mailboxName.size(); ++i)
                {
                    if (MatchListPatternRecursive(mailboxName, i, pattern, patternPos + 1))
                    {
                        return true;
                    }
                }

                return false;
            }

            if (pc == '%')
            {
                // % 匹配当前层级内的任意字符，不跨 '/'
                if (MatchListPatternRecursive(mailboxName, namePos, pattern, patternPos + 1))
                {
                    return true;
                }

                size_t i = namePos;

                while (i < mailboxName.size() && mailboxName[i] != kHierarchyDelimiter)
                {
                    ++i;

                    if (MatchListPatternRecursive(mailboxName, i, pattern, patternPos + 1))
                    {
                        return true;
                    }
                }

                return false;
            }

            if (namePos >= mailboxName.size())
            {
                return false;
            }

            if (pc != mailboxName[namePos])
            {
                return false;
            }

            return MatchListPatternRecursive(mailboxName, namePos + 1, pattern, patternPos + 1);
        }

        std::string ListCommand::BuildListAttributes(
            const Service::MailFolderInfo &folder) const
        {
            std::string attrs = "(";

            bool first = true;

            auto add = [&](const char *attr)
            {
                if (!first)
                {
                    attrs.push_back(' ');
                }

                attrs.append(attr);
                first = false;
            };

            if (!folder.selectable)
            {
                add("\\Noselect");
            }

            if (folder.hasChildren)
            {
                add("\\HasChildren");
            }
            else
            {
                add("\\HasNoChildren");
            }

            if (folder.subscribed)
            {
                add("\\Subscribed");
            }

            if (Service::HasSpecialUse(folder.specialUse, Service::ImapSpecialUse::Sent))
            {
                add("\\Sent");
            }

            if (Service::HasSpecialUse(folder.specialUse, Service::ImapSpecialUse::Drafts))
            {
                add("\\Drafts");
            }

            if (Service::HasSpecialUse(folder.specialUse, Service::ImapSpecialUse::Trash))
            {
                add("\\Trash");
            }

            if (Service::HasSpecialUse(folder.specialUse, Service::ImapSpecialUse::Junk))
            {
                add("\\Junk");
            }

            if (Service::HasSpecialUse(folder.specialUse, Service::ImapSpecialUse::Archive))
            {
                add("\\Archive");
            }

            if (Service::HasSpecialUse(folder.specialUse, Service::ImapSpecialUse::All))
            {
                add("\\All");
            }

            if (Service::HasSpecialUse(folder.specialUse, Service::ImapSpecialUse::Flagged))
            {
                add("\\Flagged");
            }

            attrs.push_back(')');
            return attrs;
        }

        std::string ListCommand::QuoteString(const std::string &value) const
        {
            std::string out;
            out.reserve(value.size() + 2);

            out.push_back('"');

            for (char ch : value)
            {
                if (ch == '\\' || ch == '"')
                {
                    out.push_back('\\');
                }

                out.push_back(ch);
            }

            out.push_back('"');
            return out;
        }

        void ListCommand::SkipSpaces(const std::string &text, size_t &pos) const
        {
            while (pos < text.size())
            {
                const unsigned char ch = static_cast<unsigned char>(text[pos]);

                if (ch != ' ' && ch != '\t')
                {
                    break;
                }

                ++pos;
            }
        }

        std::string ListCommand::ToUpperAscii(std::string_view value)
        {
            std::string out;
            out.reserve(value.size());

            for (char ch : value)
            {
                out.push_back(static_cast<char>(
                    std::toupper(static_cast<unsigned char>(ch))));
            }

            return out;
        }

        bool ListCommand::ContainsWordIgnoreCase(std::string_view text, std::string_view word)
        {
            const std::string upperText = ToUpperAscii(text);
            const std::string upperWord = ToUpperAscii(word);

            return upperText.find(upperWord) != std::string::npos;
        }

        void ListCommand::ResetState()
        {
            m_stage = Stage::None;
            m_tag.clear();

            m_args = ListArguments{};

            m_expectedLiteralBytes = 0;
            m_receivedLiteralBytes = 0;
            m_literalBuffer.clear();
        }

    }
}