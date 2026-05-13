#include <command/StatusCommand.hpp>
#include <Session.hpp>
#include <command/Command.hpp>
#include <LikesProgram/log/Logger.hpp>

#include <algorithm>
#include <cctype>
#include <string_view>

namespace NovaMail {
    namespace Imap {

        void StatusCommand::Handle(
            const std::shared_ptr<Session>& session,
            const Command& command
        ) {
            if (!session) return;

            const std::string tag(command.Tag());

            const auto* user = session->GetUser();
            if (!user || user->mailBoxId == 0) {
                session->ReplyTagged(tag, Status::No, "No available mailbox");
                return;
            }

            StatusArguments args;

            if (!ParseStatusArguments(command.Arguments(), args)) {
                session->ReplyTagged(tag, Status::Bad, "Bad STATUS arguments");
                return;
            }

            auto folder = m_folderService.GetMailboxForStatus(user->userId, user->mailBoxId, args.mailboxName);
            if (!folder || !folder->Valid()) {
                session->ReplyTagged(tag, Status::No, "[NONEXISTENT] Mailbox does not exist");
                return;
            }

            MailboxStatus status;
            if (folder->hasUidState) {
                status.uidNext = folder->uidNext;
                status.uidValidity = folder->uidValidity;
            } else { // 防止返回 0
                status.uidNext = folder->uidNext == 0 ? 1 : folder->uidNext;
                status.uidValidity = folder->uidValidity == 0 ? 1 : folder->uidValidity;
            }

            status.recent = 0;
            if (!m_mailBoxService.GetMailboxStatus(user->mailBoxId, folder->folderId, status.messages, status.unseen, status.recent)) {
                session->ReplyTagged(tag, Status::No, "STATUS failed");
                return;
            }

            session->ReplyUntagged(BuildStatusResponse(args.mailboxName, args.items, status));

            session->ReplyTagged(tag, Status::Ok, "STATUS completed");

            LogInfo(
                u"IMAP STATUS completed: userId={} mailBoxId={} mailbox={} folderId={} messages={} unseen={} recent={} uidNext={} uidValidity={}",
                static_cast<int64_t>(user->userId), static_cast<int64_t>(user->mailBoxId), args.mailboxName,
                static_cast<int64_t>(folder->folderId), static_cast<int64_t>(status.messages),
                static_cast<int64_t>(status.unseen), static_cast<int64_t>(status.recent),
                static_cast<int64_t>(status.uidNext), static_cast<int64_t>(status.uidValidity)
            );
        }

        bool StatusCommand::ParseStatusArguments(
            std::string_view args,
            StatusArguments& out
        ) const {
            out = StatusArguments{};

            size_t pos = 0;

            if (!ReadAString(args, pos, out.mailboxName)) {
                return false;
            }

            if (!ReadStatusItemList(args, pos, out.items)) {
                return false;
            }

            SkipSpaces(args, pos);

            return pos == args.size() &&
                   !out.mailboxName.empty() &&
                   !out.items.empty();
        }

        bool StatusCommand::ReadAString(
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

            const size_t begin = pos;

            while (pos < text.size()) {
                const char ch = text[pos];

                if (ch == ' ' || ch == '\t' || ch == '(') {
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

        bool StatusCommand::ReadQuotedString(
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

        bool StatusCommand::ReadStatusItemList(
            std::string_view text,
            size_t& pos,
            std::vector<std::string>& out
        ) const {
            out.clear();

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
                    return !out.empty();
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

                std::string item = ToUpperAscii(text.substr(begin, pos - begin));

                if (!IsKnownStatusItem(item)) {
                    return false;
                }

                out.emplace_back(std::move(item));
            }
        }

        std::string StatusCommand::BuildStatusResponse(const std::string& mailboxName, const std::vector<std::string>& items, const MailboxStatus& status) const {
            std::string response;

            response.append("STATUS ");
            response.append(QuoteString(mailboxName));
            response.append(" (");

            bool first = true;

            auto add = [&](const std::string& name, uint64_t value) {
                if (!first) response.push_back(' ');

                response.append(name);
                response.push_back(' ');
                response.append(std::to_string(value));

                first = false;
            };

            for (const auto& item : items) {
                if (item == "MESSAGES") {
                    add("MESSAGES", status.messages);
                } else if (item == "RECENT") {
                    add("RECENT", status.recent);
                } else if (item == "UIDNEXT") {
                    add("UIDNEXT", status.uidNext);
                } else if (item == "UIDVALIDITY") {
                    add("UIDVALIDITY", status.uidValidity);
                } else if (item == "UNSEEN") {
                    add("UNSEEN", status.unseen);
                }
            }

            response.push_back(')');

            return response;
        }

        void StatusCommand::SkipSpaces(
            std::string_view text,
            size_t& pos
        ) const noexcept {
            while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t')) {
                ++pos;
            }
        }

        bool StatusCommand::IsKnownStatusItem(std::string_view item) {
            const std::string upper = ToUpperAscii(item);

            return upper == "MESSAGES" ||
                   upper == "RECENT" ||
                   upper == "UIDNEXT" ||
                   upper == "UIDVALIDITY" ||
                   upper == "UNSEEN";
        }

        bool StatusCommand::EqualsMailboxName(
            std::string_view lhs,
            std::string_view rhs
        ) {
            /*
             * INBOX 在 IMAP 中大小写不敏感。
             * 其他 mailbox 通常大小写敏感，但为了兼容 Thunderbird 当前阶段，
             * 这里也做一次大小写无关匹配。
             */
            return ToUpperAscii(TrimCopy(lhs)) == ToUpperAscii(TrimCopy(rhs));
        }

        std::string StatusCommand::QuoteString(std::string_view value) {
            std::string out;
            out.reserve(value.size() + 2);

            out.push_back('"');

            for (char ch : value) {
                if (ch == '\\' || ch == '"') {
                    out.push_back('\\');
                }

                if (ch == '\r' || ch == '\n') {
                    out.push_back(' ');
                    continue;
                }

                out.push_back(ch);
            }

            out.push_back('"');

            return out;
        }

        std::string StatusCommand::ToUpperAscii(std::string_view value) {
            std::string out;
            out.reserve(value.size());

            for (char ch : value) {
                out.push_back(static_cast<char>(
                    std::toupper(static_cast<unsigned char>(ch))
                ));
            }

            return out;
        }

        std::string StatusCommand::TrimCopy(std::string_view value) {
            while (!value.empty() &&
                   (value.front() == ' ' ||
                    value.front() == '\t' ||
                    value.front() == '\r' ||
                    value.front() == '\n')) {
                value.remove_prefix(1);
            }

            while (!value.empty() &&
                   (value.back() == ' ' ||
                    value.back() == '\t' ||
                    value.back() == '\r' ||
                    value.back() == '\n')) {
                value.remove_suffix(1);
            }

            return std::string(value);
        }

    }
}