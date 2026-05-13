#include <command/SelectCommand.hpp>
#include <Session.hpp>
#include <command/Command.hpp>
#include <LikesProgram/log/Logger.hpp>

#include <algorithm>
#include <cctype>
#include <exception>
#include <string_view>

namespace NovaMail {
    namespace Imap {
        void SelectCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;

            ResetState();

            m_tag = std::string(command.Tag());
            m_activeVerb = command.Verb();

            const std::string args = std::string(command.Arguments());

            LiteralSpec literal;
            bool needLiteral = false;

            if (!ParseInitialArguments(args, m_mailboxName, literal, m_options, needLiteral)) {
                session->ReplyTagged(
                    m_tag,
                    Status::Bad,
                    m_readOnly ? "Bad EXAMINE arguments" : "Bad SELECT arguments"
                );
                ResetState();
                return;
            }

            if (needLiteral) {
                if (!BeginLiteralRead(session, literal)) {
                    ResetState();
                }
                return;
            }

            ExecuteSelect(session);
            ResetState();
        }

        InputResult SelectCommand::InputData(
            const std::shared_ptr<Session>& session,
            LikesProgram::Net::Buffer& in
        ) {
            if (!session) return InputResult::Failed;

            for (;;) {
                switch (m_stage) {
                    case Stage::ReadingMailboxLiteral: {
                        auto result = ReadMailboxLiteralBytes(session, in);

                        if (result == InputResult::NeedMoreData ||
                            result == InputResult::Failed ||
                            result == InputResult::Finished) {
                            return result;
                        }

                        continue;
                    }

                    case Stage::NeedRestLine:
                        return ReadRestLineAndExecute(session, in);

                    case Stage::None:
                    default:
                        return InputResult::Failed;
                }
            }
        }

        bool SelectCommand::ParseInitialArguments(
            const std::string& args,
            std::string& mailboxName,
            LiteralSpec& literal,
            SelectOptions& options,
            bool& needLiteral
        ) const {
            mailboxName.clear();
            literal = LiteralSpec{};
            options = SelectOptions{};
            needLiteral = false;

            size_t pos = 0;

            const auto mailboxResult = ReadMailboxName(args, pos, mailboxName, literal);

            if (mailboxResult == ReadStringResult::Bad) {
                return false;
            }

            if (mailboxResult == ReadStringResult::NeedLiteral) {
                SkipSpaces(args, pos);

                // mailbox literal marker 必须是当前行的最后一个参数。
                // 例如合法：
                //   SELECT {5}\r\nINBOX\r\n
                //   SELECT {5}\r\nINBOX (CONDSTORE)\r\n
                //
                // 不允许：
                //   SELECT {5} xxx
                if (pos != args.size()) {
                    return false;
                }

                needLiteral = true;
                return true;
            }

            if (mailboxName.empty()) {
                return false;
            }

            if (!ParseSelectOptions(args, pos, options)) {
                return false;
            }

            SkipSpaces(args, pos);
            return pos == args.size();
        }

        bool SelectCommand::ParseRestLineAfterLiteral(
            const std::string& line,
            SelectOptions& options
        ) const {
            size_t pos = 0;

            if (!ParseSelectOptions(line, pos, options)) {
                return false;
            }

            SkipSpaces(line, pos);
            return pos == line.size();
        }

        SelectCommand::ReadStringResult SelectCommand::ReadMailboxName(
            const std::string& text,
            size_t& pos,
            std::string& out,
            LiteralSpec& literal
        ) const {
            out.clear();
            literal = LiteralSpec{};

            SkipSpaces(text, pos);

            if (pos >= text.size()) {
                return ReadStringResult::Bad;
            }

            if (text[pos] == '{') {
                if (!ParseLiteralSpec(text, pos, literal)) {
                    return ReadStringResult::Bad;
                }

                return ReadStringResult::NeedLiteral;
            }

            if (text[pos] == '"') {
                ++pos;

                while (pos < text.size()) {
                    const char ch = text[pos++];

                    if (ch == '"') {
                        return out.empty() ? ReadStringResult::Bad : ReadStringResult::Ok;
                    }

                    if (ch == '\\') {
                        if (pos >= text.size()) {
                            return ReadStringResult::Bad;
                        }

                        const char escaped = text[pos++];

                        if (escaped == '\r' || escaped == '\n') {
                            return ReadStringResult::Bad;
                        }

                        out.push_back(escaped);
                        continue;
                    }

                    const unsigned char uch = static_cast<unsigned char>(ch);

                    if (uch == '\r' || uch == '\n') {
                        return ReadStringResult::Bad;
                    }

                    out.push_back(ch);
                }

                return ReadStringResult::Bad;
            }

            while (pos < text.size()) {
                const char ch = text[pos];

                if (ch == ' ' || ch == '\t') {
                    break;
                }

                const unsigned char uch = static_cast<unsigned char>(ch);

                if (uch < 0x20 || uch == 0x7F) {
                    return ReadStringResult::Bad;
                }

                if (ch == '(' || ch == ')' || ch == '{' || ch == '"' || ch == '\\') {
                    return ReadStringResult::Bad;
                }

                out.push_back(ch);
                ++pos;
            }

            return out.empty() ? ReadStringResult::Bad : ReadStringResult::Ok;
        }

        bool SelectCommand::ParseLiteralSpec(
            const std::string& text,
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

            const size_t numberEnd = pos;

            if (pos < text.size() && text[pos] == '+') {
                literal.nonSync = true;
                ++pos;
            }

            if (pos >= text.size() || text[pos] != '}') {
                return false;
            }

            ++pos;

            try {
                literal.size = static_cast<size_t>(
                    std::stoull(text.substr(numberBegin, numberEnd - numberBegin))
                );
            } catch (...) {
                return false;
            }

            return true;
        }

        bool SelectCommand::ParseSelectOptions(
            const std::string& text,
            size_t& pos,
            SelectOptions& options
        ) const {
            SkipSpaces(text, pos);

            if (pos >= text.size()) {
                return true;
            }

            std::string rawOptions;

            if (!ReadBalancedParentheses(text, pos, rawOptions)) {
                return false;
            }

            options.raw = rawOptions;

            if (ContainsWordIgnoreCase(rawOptions, "CONDSTORE")) {
                options.condStore = true;
            }

            if (ContainsWordIgnoreCase(rawOptions, "QRESYNC")) {
                options.qresync = true;
            }

            /*
             * 目前如果没有实现 CONDSTORE/QRESYNC，就不要静默接受。
             * 否则客户端会认为服务端支持增量同步语义。
             */
            if (options.condStore || options.qresync) {
                options.unsupported = true;
            }

            return true;
        }

        bool SelectCommand::ReadBalancedParentheses(
            const std::string& text,
            size_t& pos,
            std::string& out
        ) const {
            out.clear();

            SkipSpaces(text, pos);

            if (pos >= text.size() || text[pos] != '(') {
                return false;
            }

            const size_t begin = pos;

            int depth = 0;
            bool inQuote = false;
            bool escaped = false;

            while (pos < text.size()) {
                const char ch = text[pos++];

                if (inQuote) {
                    if (escaped) {
                        escaped = false;
                        continue;
                    }

                    if (ch == '\\') {
                        escaped = true;
                        continue;
                    }

                    if (ch == '"') {
                        inQuote = false;
                        continue;
                    }

                    if (ch == '\r' || ch == '\n') {
                        return false;
                    }

                    continue;
                }

                if (ch == '"') {
                    inQuote = true;
                    continue;
                }

                if (ch == '(') {
                    ++depth;
                    continue;
                }

                if (ch == ')') {
                    --depth;

                    if (depth == 0) {
                        out = text.substr(begin, pos - begin);
                        return true;
                    }

                    if (depth < 0) {
                        return false;
                    }

                    continue;
                }

                const unsigned char uch = static_cast<unsigned char>(ch);

                if (uch < 0x20 && ch != '\t') {
                    return false;
                }
            }

            return false;
        }

        bool SelectCommand::BeginLiteralRead(
            const std::shared_ptr<Session>& session,
            const LiteralSpec& literal
        ) {
            if (!session) return false;

            if (literal.size == 0) {
                session->ReplyTagged(
                    m_tag,
                    Status::Bad,
                    m_readOnly ? "Empty EXAMINE mailbox name" : "Empty SELECT mailbox name"
                );
                return false;
            }

            if (literal.size > kMaxMailboxLiteralBytes) {
                session->ReplyTagged(
                    m_tag,
                    Status::Bad,
                    m_readOnly ? "EXAMINE mailbox literal too large" : "SELECT mailbox literal too large"
                );
                return false;
            }

            m_stage = Stage::ReadingMailboxLiteral;
            m_expectedLiteralBytes = literal.size;
            m_receivedLiteralBytes = 0;
            m_literalBuffer.clear();
            m_literalBuffer.reserve(literal.size);

            if (!literal.nonSync) {
                session->ReplyContinuation("Ready for literal data");
            }

            /*
             * 注意：
             * SELECT 和 EXAMINE 共用 SelectCommand 类。
             * 这里必须使用 m_activeVerb，而不能写死 CommandVerb::SELECT。
             * 否则 EXAMINE 的 literal 续传会被转发到 SELECT handler。
             */
            session->BeginLiteralBytes(m_activeVerb, m_tag);
            return true;
        }

        InputResult SelectCommand::ReadMailboxLiteralBytes(
            const std::shared_ptr<Session>& session,
            LikesProgram::Net::Buffer& in
        ) {
            if (!session) return InputResult::Failed;

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

            if (m_receivedLiteralBytes < m_expectedLiteralBytes) {
                return InputResult::NeedMoreData;
            }

            m_mailboxName = std::move(m_literalBuffer);
            m_literalBuffer.clear();

            if (m_mailboxName.empty()) {
                session->ReplyTagged(
                    m_tag,
                    Status::Bad,
                    m_readOnly ? "Empty EXAMINE mailbox name" : "Empty SELECT mailbox name"
                );
                ResetState();
                return InputResult::Finished;
            }

            /*
             * literal 内容只包含 mailboxName。
             * 后面还可能有：
             *   \r\n
             *   " (CONDSTORE)\r\n"
             *   " (QRESYNC (...))\r\n"
             *
             * 所以不能直接 ExecuteSelect，需要继续读剩余行。
             */
            m_stage = Stage::NeedRestLine;
            return InputResult::Continue;
        }

        InputResult SelectCommand::ReadRestLineAndExecute(
            const std::shared_ptr<Session>& session,
            LikesProgram::Net::Buffer& in
        ) {
            if (!session) return InputResult::Failed;

            const size_t readable = in.ReadableBytes();

            if (readable == 0) {
                return InputResult::NeedMoreData;
            }

            const auto* data = in.Peek();

            size_t lfPos = static_cast<size_t>(-1);

            for (size_t i = 0; i < readable; ++i) {
                if (data[i] == '\n') {
                    lfPos = i;
                    break;
                }
            }

            if (lfPos == static_cast<size_t>(-1)) {
                if (readable > kMaxSelectRestLineBytes) {
                    session->ReplyTagged(
                        m_tag,
                        Status::Bad,
                        m_readOnly ? "EXAMINE continuation line too large" : "SELECT continuation line too large"
                    );
                    ResetState();
                    return InputResult::Finished;
                }

                return InputResult::NeedMoreData;
            }

            size_t lineLen = lfPos;

            if (lineLen > 0 && data[lineLen - 1] == '\r') {
                --lineLen;
            }

            std::string line(reinterpret_cast<const char*>(data), lineLen);
            in.Consume(lfPos + 1);

            if (!ParseRestLineAfterLiteral(line, m_options)) {
                session->ReplyTagged(
                    m_tag,
                    Status::Bad,
                    m_readOnly ? "Bad EXAMINE continuation" : "Bad SELECT continuation"
                );
                ResetState();
                return InputResult::Finished;
            }

            ExecuteSelect(session);
            ResetState();
            return InputResult::Finished;
        }

        void SelectCommand::ExecuteSelect(const std::shared_ptr<Session>& session) {
            if (!session) return;

            const auto* user = session->GetUser();

            if (!user) {
                session->ReplyTagged(m_tag, Status::Bad, "Not authenticated");
                return;
            }

            if (HasUnsupportedSelectOptions(m_options)) {
                session->ReplyTagged(m_tag, Status::Bad, m_readOnly ? "Unsupported EXAMINE parameters" : "Unsupported SELECT parameters");
                return;
            }

            const uint64_t userId = user->userId;
            const uint64_t mailBoxId = user->mailBoxId;

            if (mailBoxId == 0) {
                session->ReplyTagged(m_tag, Status::No, "No available mailbox");
                return;
            }

            auto mailbox = m_mailFolderService.GetMailboxForStatus(userId, mailBoxId, m_mailboxName);

            if (!mailbox || !mailbox->Valid()) {
                session->ReplyTagged(
                    m_tag,
                    Status::No,
                    m_readOnly ? "EXAMINE mailbox does not exist" : "SELECT mailbox does not exist"
                );
                return;
            }

            if (!mailbox->selectable) {
                session->ReplyTagged(
                    m_tag,
                    Status::No,
                    "Mailbox is not selectable"
                );
                return;
            }

            const auto status = LoadMailboxStatus(mailBoxId, *mailbox);

            session->SelectMailbox(*mailbox, m_readOnly, status.exists);

            session->ReplyUntagged("FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)");

            session->ReplyUntagged(std::to_string(status.exists) + " EXISTS");

            if (status.firstUnseenSeq > 0) {
                session->ReplyUntagged(
                    "OK [UNSEEN " + std::to_string(status.firstUnseenSeq) + "] First unseen message"
                );
            }

            session->ReplyUntagged(
                "OK [UIDVALIDITY " + std::to_string(status.uidValidity) + "] UIDs valid"
            );

            session->ReplyUntagged(
                "OK [UIDNEXT " + std::to_string(status.uidNext) + "] Predicted next UID"
            );

            if (m_readOnly) {
                session->ReplyUntagged(
                    "OK [PERMANENTFLAGS ()] No permanent flags permitted"
                );

                session->ReplyTagged(
                    m_tag,
                    Status::Ok,
                    "[READ-ONLY] EXAMINE completed"
                );
            } else {
                session->ReplyUntagged(
                    "OK [PERMANENTFLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft \\*)] Permanent flags"
                );

                session->ReplyTagged(
                    m_tag,
                    Status::Ok,
                    "[READ-WRITE] SELECT completed"
                );
            }

            LogInfo(
                u"IMAP {} completed: userId={} mailbox={} folderId={} exists={} client={} -> server={}",
                m_readOnly ? u"EXAMINE" : u"SELECT",
                static_cast<int64_t>(userId),
                LikesProgram::String(mailbox->imapName),
                static_cast<int64_t>(mailbox->folderId),
                static_cast<int64_t>(status.exists),
                session->GetRemoteAddress().ToString(),
                session->GetLocalAddress().ToString()
            );
        }

        SelectCommand::MailboxStatus SelectCommand::LoadMailboxStatus(uint64_t mailBoxId, const Service::MailFolderInfo& folder) {
            MailboxStatus status;

            status.uidNext = folder.uidNext;
            status.uidValidity = folder.uidValidity;
            status.firstUnseenSeq = 0;
            status.exists = 0;

            if (mailBoxId == 0 || !folder.Valid() || !folder.hasUidState) {
                return status;
            }

            auto counter = m_mailBoxService.LoadImapMailboxCounter(mailBoxId, folder.folderId);
            if (!counter.success) {
                LogWarn(
                    u"LoadMailboxStatus failed: mailbox counter unavailable, mailBoxId={} folderId={}",
                    static_cast<int64_t>(mailBoxId), static_cast<int64_t>(folder.folderId)
                );
                return status;
            }

            status.exists = counter.exists;
            status.firstUnseenSeq = counter.firstUnseenSeq;

            return status;
        }

        bool SelectCommand::HasUnsupportedSelectOptions(
            const SelectOptions& options
        ) const noexcept {
            return options.unsupported;
        }

        void SelectCommand::SkipSpaces(const std::string& text, size_t& pos) const {
            while (pos < text.size()) {
                const unsigned char ch = static_cast<unsigned char>(text[pos]);

                if (ch != ' ' && ch != '\t') {
                    break;
                }

                ++pos;
            }
        }

        std::string SelectCommand::ToUpperAscii(std::string_view value) {
            std::string out;
            out.reserve(value.size());

            for (char ch : value) {
                out.push_back(static_cast<char>(
                    std::toupper(static_cast<unsigned char>(ch))
                ));
            }

            return out;
        }

        bool SelectCommand::ContainsWordIgnoreCase(
            std::string_view text,
            std::string_view word
        ) {
            const std::string upperText = ToUpperAscii(text);
            const std::string upperWord = ToUpperAscii(word);

            return upperText.find(upperWord) != std::string::npos;
        }

        void SelectCommand::ResetState() {
            m_stage = Stage::None;
            m_activeVerb = CommandVerb::Unknown;

            m_tag.clear();
            m_mailboxName.clear();
            m_options = SelectOptions{};

            m_expectedLiteralBytes = 0;
            m_receivedLiteralBytes = 0;
            m_literalBuffer.clear();
        }

    }
}
