#include <command/StoreCommand.hpp>
#include <Session.hpp>
#include <command/Command.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <algorithm>
#include <cctype>
#include <limits>
#include <string_view>

namespace NovaMail {
    namespace Imap {

        void StoreCommand::Handle(
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

            const auto* selected = session->GetSelectedMailbox();
            if (!selected || !selected->Valid()) {
                session->ReplyTagged(tag, Status::Bad, "No mailbox selected");
                return;
            }

            StoreArguments args;

            if (!ParseStoreArguments(command.Arguments(), args)) {
                session->ReplyTagged(tag, Status::Bad, "Bad STORE arguments");
                return;
            }

            const uint64_t mailBoxId = user->mailBoxId;
            const uint64_t folderId = selected->folderId;
            const uint64_t selectedExists = session->GetSelectedExists();

            std::vector<uint64_t> sequences;
            std::vector<IdRange> uidRanges;

            if (!ParseIdSet(
                    args.idSet,
                    command.UidMode(),
                    selectedExists,
                    sequences,
                    uidRanges
                )) {
                session->ReplyTagged(tag, Status::Bad, "Bad STORE message set");
                return;
            }

            const uint64_t highestUid =
                selected->uidNext > 1 ? selected->uidNext - 1 : 0;

            auto processMessage = [&](uint64_t sequence) -> bool {
                auto message = m_mailBoxService.GetMessageBySequence(
                    mailBoxId,
                    folderId,
                    sequence
                );

                if (!message || !message->success) {
                    return true;
                }

                if (command.UidMode() &&
                    !UidInRanges(message->imapUid, uidRanges, highestUid)) {
                    return true;
                }

                auto currentFlags = BuildCurrentFlags(*message);

                ApplyFlagOperation(currentFlags, args.flags, args.mode);

                if (!m_mailBoxService.UpdateMessageFlags(
                        message->mappingId,
                        currentFlags
                    )) {
                    session->ReplyTagged(tag, Status::No, "STORE failed");
                    return false;
                }

                if (!args.silent) {
                    std::string response;
                    response.reserve(160);

                    response.append(std::to_string(sequence));
                    response.append(" FETCH (FLAGS ");
                    response.append(BuildFlagsResponse(currentFlags));

                    /*
                     * 对 UID STORE，untagged FETCH 里附带 UID 更稳。
                     * 注意：FETCH 响应开头的数字仍然是 sequence number，不是 UID。
                     */
                    if (command.UidMode()) {
                        response.append(" UID ");
                        response.append(std::to_string(message->imapUid));
                    }

                    response.push_back(')');

                    session->ReplyUntagged(response);
                }

                LogInfo(
                    u"IMAP {}STORE item completed: mailBoxId={} folderId={} seq={} uid={} mappingId={}",
                    command.UidMode() ? "UID " : "",
                    static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId),
                    static_cast<int64_t>(sequence),
                    static_cast<int64_t>(message->imapUid),
                    static_cast<int64_t>(message->mappingId)
                );

                return true;
            };

            if (command.UidMode()) {
                /*
                 * UID STORE 的 id-set 是 UID 范围，但 untagged FETCH 响应必须用 sequence。
                 * 所以这里遍历当前 mailbox sequence，再按 UID 范围过滤。
                 */
                for (uint64_t sequence = 1; sequence <= selectedExists; ++sequence) {
                    if (!processMessage(sequence)) {
                        return;
                    }
                }
            } else {
                for (uint64_t sequence : sequences) {
                    if (!processMessage(sequence)) {
                        return;
                    }
                }
            }

            session->ReplyTagged(
                tag,
                Status::Ok,
                command.UidMode() ? "UID STORE completed" : "STORE completed"
            );
        }

        bool StoreCommand::ParseStoreArguments(
            std::string_view args,
            StoreArguments& out
        ) const {
            out = StoreArguments{};

            const std::string text = TrimCopy(args);
            if (text.empty()) {
                return false;
            }

            size_t pos = 0;

            while (pos < text.size() && text[pos] != ' ' && text[pos] != '\t') {
                ++pos;
            }

            if (pos == 0) {
                return false;
            }

            out.idSet = text.substr(0, pos);

            SkipSpaces(text, pos);

            const size_t opBegin = pos;

            while (pos < text.size() && text[pos] != ' ' && text[pos] != '\t') {
                ++pos;
            }

            if (pos == opBegin) {
                return false;
            }

            const std::string opToken = text.substr(opBegin, pos - opBegin);

            if (!ParseStoreOperation(opToken, out.mode, out.silent)) {
                return false;
            }

            if (!ReadFlagList(text, pos, out.flags)) {
                return false;
            }

            SkipSpaces(text, pos);

            return pos == text.size();
        }

        bool StoreCommand::ParseStoreOperation(
            std::string_view token,
            StoreMode& mode,
            bool& silent
        ) const {
            std::string upper = ToUpperAscii(token);

            silent = false;

            const std::string suffix = ".SILENT";

            if (upper.size() >= suffix.size() &&
                upper.compare(upper.size() - suffix.size(), suffix.size(), suffix) == 0) {
                silent = true;
                upper.erase(upper.size() - suffix.size());
            }

            if (upper == "FLAGS") {
                mode = StoreMode::Replace;
                return true;
            }

            if (upper == "+FLAGS") {
                mode = StoreMode::Add;
                return true;
            }

            if (upper == "-FLAGS") {
                mode = StoreMode::Remove;
                return true;
            }

            return false;
        }

        bool StoreCommand::ReadFlagList(
            std::string_view text,
            size_t& pos,
            std::vector<std::string>& flags
        ) const {
            flags.clear();

            SkipSpaces(text, pos);

            if (pos >= text.size()) {
                return false;
            }

            /*
             * 标准形式：(\Seen \Flagged)
             * 同时兼容单个裸 flag：\Seen
             */
            if (text[pos] != '(') {
                const size_t begin = pos;

                while (pos < text.size() && text[pos] != ' ' && text[pos] != '\t') {
                    ++pos;
                }

                if (pos == begin) {
                    return false;
                }

                flags.emplace_back(text.substr(begin, pos - begin));
                return true;
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

        bool StoreCommand::ParseIdSet(
            std::string_view value,
            bool uidMode,
            uint64_t selectedExists,
            std::vector<uint64_t>& sequences,
            std::vector<IdRange>& uidRanges
        ) const {
            sequences.clear();
            uidRanges.clear();

            value = TrimCopy(value);

            if (value.empty()) {
                return false;
            }

            constexpr uint64_t kStar = std::numeric_limits<uint64_t>::max();

            auto parseOne = [&](std::string_view token, uint64_t& out) -> bool {
                token = TrimCopy(token);

                if (token.empty()) {
                    return false;
                }

                if (token == "*") {
                    if (uidMode) {
                        out = kStar;
                        return true;
                    }

                    if (selectedExists == 0) {
                        return false;
                    }

                    out = selectedExists;
                    return true;
                }

                uint64_t number = 0;

                for (char ch : token) {
                    if (!std::isdigit(static_cast<unsigned char>(ch))) {
                        return false;
                    }

                    const uint64_t digit = static_cast<uint64_t>(ch - '0');

                    if (number > (std::numeric_limits<uint64_t>::max() - digit) / 10) {
                        return false;
                    }

                    number = number * 10 + digit;
                }

                if (number == 0) {
                    return false;
                }

                out = number;
                return true;
            };

            size_t begin = 0;

            for (;;) {
                const size_t comma = value.find(',', begin);

                const std::string part = std::string(
                    value.substr(
                        begin,
                        comma == std::string_view::npos
                            ? std::string_view::npos
                            : comma - begin
                    )
                );

                if (part.empty()) return false;

                const size_t colon = part.find(':');

                if (colon == std::string::npos) {
                    uint64_t one = 0;

                    if (!parseOne(part, one)) return false;

                    if (uidMode) {
                        uidRanges.push_back(IdRange{one, one});
                    } else {
                        if (one <= selectedExists)  sequences.push_back(one);
                    }
                } else {
                    uint64_t first = 0;
                    uint64_t last = 0;

                    if (!parseOne(std::string_view(part).substr(0, colon), first)) return false;

                    if (!parseOne(std::string_view(part).substr(colon + 1), last)) return false;

                    if (uidMode) {
                        uidRanges.push_back(IdRange{first, last});
                    } else {
                        if (first > last) {
                            std::swap(first, last);
                        }

                        if (selectedExists > 0) {
                            last = std::min(last, selectedExists);
                        }

                        for (uint64_t sequence = first; sequence <= last; ++sequence) {
                            sequences.push_back(sequence);
                        }
                    }
                }

                if (comma == std::string_view::npos) {
                    break;
                }

                begin = comma + 1;
            }

            if (uidMode) {
                return !uidRanges.empty();
            }

            std::sort(sequences.begin(), sequences.end());
            sequences.erase(
                std::unique(sequences.begin(), sequences.end()),
                sequences.end()
            );

            return true;
        }

        bool StoreCommand::UidInRanges(uint64_t uid, const std::vector<IdRange>& ranges, uint64_t highestUid) const {
            constexpr uint64_t kStar = std::numeric_limits<uint64_t>::max();

            for (const auto& range : ranges) {
                uint64_t first = range.first;
                uint64_t last = range.last;
                if (first == kStar && last == kStar) return uid == highestUid;
                if (first == kStar) first = highestUid;
                if (last == kStar) last = highestUid;
                if (first > last) std::swap(first, last);
                if (uid >= first && uid <= last) return true;
            }

            return false;
        }

        Service::ImapMessageFlag StoreCommand::BuildCurrentFlags(const Service::MailBoxService::ImapFetchMessage& message) const {
            using Service::ImapMessageFlag;

            ImapMessageFlag flags = ImapMessageFlag::None;

            if (message.isRead) flags |= ImapMessageFlag::Seen;
            if (message.isStarred) flags |= ImapMessageFlag::Flagged;
            if (message.isAnswered) flags |= ImapMessageFlag::Answered;
            return flags;
        }

        void StoreCommand::ApplyFlagOperation(
            Service::ImapMessageFlag& target,
            const std::vector<std::string>& flags,
            StoreMode mode
        ) const {
            using Service::ImapMessageFlag;
            ImapMessageFlag requested = ImapMessageFlag::None;

            for (const auto& rawFlag : flags) {
                const std::string flag = ToUpperAscii(rawFlag);
                if (flag == "\\SEEN") {
                    requested |= ImapMessageFlag::Seen;
                } else if (flag == "\\FLAGGED") {
                    requested |= ImapMessageFlag::Flagged;
                } else if (flag == "\\ANSWERED") {
                    requested |= ImapMessageFlag::Answered;
                } else if (flag == "\\DELETED") {
                    /*
                    * 当前系统用文件夹区分删除状态，不在 MailBoxMailMapping 上保存 \Deleted。
                    * 后续删除动作建议通过 MOVE / COPY + STORE / EXPUNGE 组合处理。
                    */
                } else if (flag == "\\DRAFT") {
                    /*
                    * 当前系统用 Drafts 文件夹区分草稿，不保存 \Draft flag。
                    */
                }
            }

            if (mode == StoreMode::Replace) {
                target = requested;
                return;
            }

            if (mode == StoreMode::Add) {
                target |= requested;
                return;
            }

            if (mode == StoreMode::Remove) {
                target = static_cast<ImapMessageFlag>(
                    static_cast<uint32_t>(target) &
                    ~static_cast<uint32_t>(requested)
                );
            }
        }

        std::string StoreCommand::BuildFlagsResponse(const Service::ImapMessageFlag& flags) const {
            using Service::HasMessageFlag;
            using Service::ImapMessageFlag;

            std::string out = "(";

            bool first = true;

            auto add = [&](const char* flag) {
                if (!first) out.push_back(' ');
                out.append(flag);
                first = false;
            };

            if (HasMessageFlag(flags, ImapMessageFlag::Answered)) add("\\Answered");
            if (HasMessageFlag(flags, ImapMessageFlag::Flagged)) add("\\Flagged");
            if (HasMessageFlag(flags, ImapMessageFlag::Seen)) add("\\Seen");
            out.push_back(')');

            return out;
        }

        void StoreCommand::SkipSpaces(std::string_view text, size_t& pos) const noexcept {
            while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t')) {
                ++pos;
            }
        }

        std::string StoreCommand::ToUpperAscii(std::string_view value) {
            std::string out;
            out.reserve(value.size());

            for (char ch : value) {
                out.push_back(static_cast<char>(
                    std::toupper(static_cast<unsigned char>(ch))
                ));
            }

            return out;
        }

        std::string StoreCommand::TrimCopy(std::string_view value) {
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