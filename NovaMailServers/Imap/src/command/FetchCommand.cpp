#include <command/FetchCommand.hpp>
#include <Session.hpp>
#include <command/Command.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

namespace NovaMail {
    namespace Imap {
        static std::string FormatImapInternalDate(std::time_t t) {
            std::tm tm{};
            gmtime_r(&t, &tm);
            static constexpr const char* kMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%02d-%s-%04d %02d:%02d:%02d +0000", tm.tm_mday, kMonths[tm.tm_mon], tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
        
            return buffer;
        }
        FetchCommand::FetchCommand() : m_mailBoxService(LikeHub::Storage::DatabasePool::Instance()), m_emailService(LikeHub::Storage::DatabasePool::Instance()) { }

        void FetchCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
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

            FetchRequest request;
            request.uidMode = command.UidMode();
            if (!ParseFetchRequest(command.Arguments(), session->GetSelectedExists(), command.UidMode(), request)) {
                session->ReplyTagged(tag, Status::Bad, "Bad FETCH arguments");
                return;
            }

            const uint64_t mailBoxId = user->mailBoxId;
            const uint64_t folderId = selected->folderId;
            const uint64_t selectedExists = session->GetSelectedExists();
            LogInfo(
                u"IMAP {} requested: tag={} args={} selectedExists={}",
                request.uidMode ? u"UID FETCH" : u"FETCH", tag,
                std::string(command.Arguments()), static_cast<int64_t>(session->GetSelectedExists())
            );
            auto processMessage = [&](uint64_t sequence) -> bool {
                auto message = m_mailBoxService.GetMessageBySequence(mailBoxId, folderId, sequence);

                if (!message || !message->success) return true;
                if (request.uidMode && !UidInRanges(message->imapUid, request.uidRanges)) return true;

                uint64_t mailSize = 0;
                if (!m_emailService.GetMailSize(message->mailId, mailSize)) {
                    session->ReplyTagged(tag, Status::No, "Message size unavailable");
                    return false;
                }
            
                LikesProgram::Net::Buffer mailData;
                ParsedMailView mailView;
            
                if (request.requiresMailData) {
                    if (!LoadMessageData(message->mailId, mailData)) {
                        session->ReplyTagged(tag, Status::No, "Message body unavailable");
                        return false;
                    }
                
                    mailView = BuildMailView(mailData);
                
                    if (mailData.ReadableBytes() != mailSize) {
                        LogWarn(
                            u"FETCH mail size mismatch: mailId={} expected={} actual={}",
                            static_cast<int64_t>(message->mailId),
                            static_cast<int64_t>(mailSize),
                            static_cast<int64_t>(mailData.ReadableBytes())
                        );
                    }
                
                    mailSize = static_cast<uint64_t>(mailData.ReadableBytes());
                }
            
                if (request.shouldMarkSeen && !message->isRead) {
                    if (m_mailBoxService.MarkSeen(message->mappingId)) {
                        message->isRead = true;
                    }
                }

                std::string response = BuildFetchResponse(sequence, *message, request, request.requiresMailData ? &mailView : nullptr, static_cast<size_t>(mailSize));

                session->ReplyUntagged(response);

                LogInfo(
                    u"IMAP {} item completed: mailBoxId={} folderId={} seq={} uid={} mailId={} size={}",
                    request.uidMode ? u"UID FETCH" : u"FETCH", static_cast<int64_t>(mailBoxId),
                    static_cast<int64_t>(folderId), static_cast<int64_t>(sequence),
                    static_cast<int64_t>(message->imapUid), static_cast<int64_t>(message->mailId),
                    static_cast<int64_t>(mailSize)
                );
            
                return true;
            };

            if (request.uidMode) { for (uint64_t sequence = 1; sequence <= selectedExists; ++sequence) if (!processMessage(sequence)) return; }
            else for (uint64_t sequence : request.sequences) if (!processMessage(sequence)) return;
        
            session->ReplyTagged(tag, Status::Ok, request.uidMode ? "UID FETCH completed" : "FETCH completed");
        }

        bool FetchCommand::ParseFetchRequest(std::string_view args, uint64_t selectedExists, bool uidMode, FetchRequest& request) const {
            request = FetchRequest{};
            request.uidMode = uidMode;

            const std::string text = TrimCopy(args);
            if (text.empty()) return false;

            size_t pos = 0;
            while (pos < text.size() && text[pos] != ' ' && text[pos] != '\t') ++pos;
            if (pos == 0) return false;

            const std::string sequenceSet = text.substr(0, pos);
            if (!ParseSequenceSet(sequenceSet, selectedExists, uidMode, request.sequences, request.uidRanges)) return false;

            while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t')) ++pos;
            if (pos >= text.size()) return false;
            if (!ParseFetchItems(std::string_view(text).substr(pos), request)) return false;

            if (request.uidMode) EnsureUidItem(request);
            return true;
        }

        bool FetchCommand::ParseSequenceSet(std::string_view value, uint64_t selectedExists, bool uidMode, std::vector<uint64_t>& sequences, std::vector<FetchRange>& uidRanges) const {
            sequences.clear();
            uidRanges.clear();

            if (value.empty()) return false;

            constexpr uint64_t kStar = std::numeric_limits<uint64_t>::max();

            auto parseOne = [&](std::string_view token, uint64_t& out) -> bool {
                token = TrimCopy(token);

                if (token.empty()) return false;

                if (token == "*") {
                    if (uidMode) {
                        out = kStar;
                        return true;
                    }

                    if (selectedExists == 0) return false;
                    out = selectedExists;
                    return true;
                }

                uint64_t number = 0;

                for (char ch : token) {
                    if (!std::isdigit(static_cast<unsigned char>(ch))) return false;

                    const uint64_t digit = static_cast<uint64_t>(ch - '0');

                    if (number > (std::numeric_limits<uint64_t>::max() - digit) / 10) return false;

                    number = number * 10 + digit;
                }

                if (number == 0) return false;

                out = number;
                return true;
            };

            size_t begin = 0;

            for (;;) {
                const size_t comma = value.find(',', begin);

                const std::string part = std::string(
                    value.substr(begin, comma == std::string_view::npos ? std::string_view::npos : comma - begin)
                );

                const size_t colon = part.find(':');

                if (colon == std::string::npos) {
                    uint64_t one = 0;
                    if (!parseOne(part, one)) return false;

                    if (uidMode) {
                        uidRanges.push_back(FetchRange{one, one});
                    } else {
                        if (one <= selectedExists) sequences.push_back(one);
                    }
                } else {
                    uint64_t first = 0;
                    uint64_t last = 0;

                    if (!parseOne(std::string_view(part).substr(0, colon), first)) return false;

                    if (!parseOne(std::string_view(part).substr(colon + 1), last)) return false;

                    if (first != kStar && last != kStar && first > last) std::swap(first, last);

                    if (uidMode) {
                        uidRanges.push_back(FetchRange{first, last});
                    } else {
                        if (first > last)  std::swap(first, last);

                        if (selectedExists > 0) last = std::min(last, selectedExists);

                        for (uint64_t seq = first; seq <= last; ++seq) sequences.push_back(seq);
                    }
                }

                if (comma == std::string_view::npos) break;

                begin = comma + 1;
            }

            if (uidMode) return !uidRanges.empty();

            std::sort(sequences.begin(), sequences.end());
            sequences.erase(std::unique(sequences.begin(), sequences.end()), sequences.end());

            return !sequences.empty();
        }

        bool FetchCommand::UidInRanges(uint64_t uid, const std::vector<FetchRange>& ranges) const {
            constexpr uint64_t kStar = std::numeric_limits<uint64_t>::max();
        
            for (const auto& range : ranges) {
                uint64_t first = range.first;
                uint64_t last = range.last;
            
                if (first == kStar && last == kStar) {
                    if (uid == kStar) return true;
                    continue;
                }
            
                if (first == kStar) {
                    first = last;
                    last = kStar;
                }
            
                if (last == kStar) {
                    if (uid >= first) return true;
                    continue;
                }
            
                if (first > last) std::swap(first, last);
            
                if (uid >= first && uid <= last) return true;
            }
        
            return false;
        }

        void FetchCommand::EnsureUidItem(FetchRequest& request) const {
            for (const auto& item : request.items) if (item.type == FetchItemType::Uid) return;

            FetchItem item;
            item.type = FetchItemType::Uid;
            item.peek = true;
            item.responseName = "UID";

            request.items.insert(request.items.begin(), std::move(item));
        }

        bool FetchCommand::ParseFetchItems(std::string_view value, FetchRequest& request) const {
            std::string itemText = TrimCopy(value);
            if (itemText.empty()) return false;

            const bool wrapped =
                itemText.size() >= 2 &&
                itemText.front() == '(' &&
                itemText.back() == ')';

            if (wrapped) {
                itemText = TrimCopy(std::string_view(itemText).substr(1, itemText.size() - 2));
            }

            const std::string upper = ToUpperAscii(itemText);
            auto isAtomBoundary = [](char ch) -> bool {
                return ch == ' ' ||
                       ch == '\t' ||
                       ch == '(' ||
                       ch == ')' ||
                       ch == '\r' ||
                       ch == '\n';
            };

            auto containsFetchAtom = [&](std::string_view atom) -> bool {
                const std::string upperAtom = ToUpperAscii(atom);
            
                size_t pos = 0;
            
                for (;;) {
                    pos = upper.find(upperAtom, pos);
                
                    if (pos == std::string::npos) {
                        return false;
                    }
                
                    const bool leftOk =
                        pos == 0 ||
                        isAtomBoundary(upper[pos - 1]);
                
                    const size_t end = pos + upperAtom.size();
                
                    const bool rightOk =
                        end >= upper.size() ||
                        isAtomBoundary(upper[end]);
                
                    if (leftOk && rightOk) {
                        return true;
                    }
                
                    ++pos;
                }
            };

            auto addSimple = [&](FetchItemType type) {
                FetchItem item;
                item.type = type;
                item.peek = true;

                switch (type) {
                    case FetchItemType::Flags:
                        item.responseName = "FLAGS";
                        break;
                
                    case FetchItemType::Uid:
                        item.responseName = "UID";
                        break;
                
                    case FetchItemType::Rfc822Size:
                        item.responseName = "RFC822.SIZE";
                        break;
                
                    case FetchItemType::InternalDate:
                        item.responseName = "INTERNALDATE";
                        break;
                
                    case FetchItemType::Envelope:
                        item.responseName = "ENVELOPE";
                        request.requiresMailData = true;
                        break;
                
                    case FetchItemType::BodyStructure:
                        item.responseName = "BODYSTRUCTURE";
                        request.requiresMailData = true;
                        break;
                
                    default:
                        break;
                }
            
                request.items.push_back(std::move(item));
            };

            auto addBody = [&](FetchItemType type, bool peek, std::string responseName, std::vector<std::string> fields = {}) {
                FetchItem item;
                item.type = type;
                item.peek = peek;
                item.responseName = std::move(responseName);
                item.headerFields = std::move(fields);

                request.requiresMailData = true;

                if (!peek) {
                    request.shouldMarkSeen = true;
                }

                request.items.push_back(std::move(item));
            };

            if (upper == "FAST") {
                addSimple(FetchItemType::Flags);
                addSimple(FetchItemType::InternalDate);
                addSimple(FetchItemType::Rfc822Size);
                return true;
            }

            if (upper == "ALL") {
                addSimple(FetchItemType::Flags);
                addSimple(FetchItemType::InternalDate);
                addSimple(FetchItemType::Rfc822Size);
                addSimple(FetchItemType::Envelope);
                return true;
            }

            if (upper == "FULL") {
                addSimple(FetchItemType::Flags);
                addSimple(FetchItemType::InternalDate);
                addSimple(FetchItemType::Rfc822Size);
                addSimple(FetchItemType::Envelope);
                addSimple(FetchItemType::BodyStructure);
                return true;
            }

            /*
             * 兼容性解析策略：
             * 不做完整 IMAP 词法分析，而是识别常见 FETCH item。
             * 这样能够覆盖 curl / Thunderbird / Outlook 常见请求。
             */
            if (containsFetchAtom("FLAGS")) {
                addSimple(FetchItemType::Flags);
            }

            if (containsFetchAtom("UID")) {
                addSimple(FetchItemType::Uid);
            }

            if (containsFetchAtom("RFC822.SIZE")) {
                addSimple(FetchItemType::Rfc822Size);
            }

            if (containsFetchAtom("INTERNALDATE")) {
                addSimple(FetchItemType::InternalDate);
            }

            if (containsFetchAtom("ENVELOPE")) {
                addSimple(FetchItemType::Envelope);
            }

            if (containsFetchAtom("BODYSTRUCTURE")) {
                addSimple(FetchItemType::BodyStructure);
            }

            if (containsFetchAtom("RFC822.HEADER")) {
                addBody(FetchItemType::BodyHeader, true, "RFC822.HEADER");
            }

            if (containsFetchAtom("RFC822.TEXT")) {
                addBody(FetchItemType::BodyText, false, "RFC822.TEXT");
            }

            if (containsFetchAtom("RFC822.PEEK")) {
                addBody(FetchItemType::BodyFull, true, "RFC822");
            }

            if (containsFetchAtom("RFC822")) {
                addBody(FetchItemType::BodyFull, false, "RFC822");
            }

            const bool hasBodyPeek = ContainsIgnoreCase(itemText, "BODY.PEEK[");
            const bool hasBody = ContainsIgnoreCase(itemText, "BODY[");

            if (hasBodyPeek || hasBody) {
                const bool peek = hasBodyPeek;

                if (ContainsIgnoreCase(itemText, "HEADER.FIELDS")) {
                    std::vector<std::string> fields;

                    const size_t fieldsPos = ToUpperAscii(itemText).find("HEADER.FIELDS");
                    const size_t open = itemText.find('(', fieldsPos);
                    const size_t close = itemText.find(')', open == std::string::npos ? fieldsPos : open);

                    if (open != std::string::npos && close != std::string::npos && close > open) {
                        const std::string fieldList = itemText.substr(open + 1, close - open - 1);
                        std::istringstream iss(fieldList);
                        std::string field;

                        while (iss >> field) {
                            fields.push_back(ToUpperAscii(field));
                        }
                    }

                    addBody(
                        FetchItemType::BodyHeaderFields,
                        peek,
                        peek ? "BODY[HEADER.FIELDS]" : "BODY[HEADER.FIELDS]",
                        std::move(fields)
                    );
                } else if (ContainsIgnoreCase(itemText, "[HEADER]")) {
                    addBody(
                        FetchItemType::BodyHeader,
                        peek,
                        peek ? "BODY[HEADER]" : "BODY[HEADER]"
                    );
                } else if (ContainsIgnoreCase(itemText, "[TEXT]")) {
                    addBody(
                        FetchItemType::BodyText,
                        peek,
                        peek ? "BODY[TEXT]" : "BODY[TEXT]"
                    );
                } else if (ContainsIgnoreCase(itemText, "BODY.PEEK[]")) {
                    addBody(FetchItemType::BodyFull, true, "BODY[]");
                } else if (ContainsIgnoreCase(itemText, "BODY[]")) {
                    addBody(FetchItemType::BodyFull, false, "BODY[]");
                }
            }

            /*
             * 如果客户端只请求 BODY，不带 []，按 BODYSTRUCTURE 处理。
             */
            if (request.items.empty() && upper == "BODY") {
                addSimple(FetchItemType::BodyStructure);
            }

            return !request.items.empty();
        }

        bool FetchCommand::LoadMessageData(
            uint64_t mailId,
            LikesProgram::Net::Buffer& mailData
        ) {
            if (mailId == 0) return false;

            if (!m_emailService.GetMailData(mailId, mailData)) {
                LogWarn(
                    u"FETCH load mail data failed: mailId={}",
                    static_cast<int64_t>(mailId)
                );
                return false;
            }

            if (mailData.ReadableBytes() == 0) {
                LogWarn(
                    u"FETCH load mail data empty: mailId={}",
                    static_cast<int64_t>(mailId)
                );
                return false;
            }

            return true;
        }

        FetchCommand::ParsedMailView FetchCommand::BuildMailView(const LikesProgram::Net::Buffer& mailData) const {
            ParsedMailView view;

            const size_t readable = mailData.ReadableBytes();

            if (readable > 0) {
                view.raw.assign(
                    reinterpret_cast<const char*>(mailData.Peek()),
                    readable
                );
            }

            size_t split = view.raw.find("\r\n\r\n");
            size_t sepLen = 4;

            if (split == std::string::npos) {
                split = view.raw.find("\n\n");
                sepLen = 2;
            }

            if (split == std::string::npos) {
                view.header = view.raw;
                view.body.clear();
                return view;
            }

            view.header = view.raw.substr(0, split + sepLen);
            view.body = view.raw.substr(split + sepLen);

            return view;
        }

        std::string FetchCommand::BuildFetchResponse(
            uint64_t sequence,
            const Service::MailBoxService::ImapFetchMessage& message,
            const FetchRequest& request,
            const ParsedMailView* mailView,
            size_t rawSize
        ) const {
            std::string response;
            response.reserve(rawSize + 512);

            response.append(std::to_string(sequence));
            response.append(" FETCH (");

            bool first = true;

            for (const auto& item : request.items) {
                if (!first) response.push_back(' ');
                AppendFetchItem(response, item, message, mailView, rawSize);
                first = false;
            }

            response.push_back(')');

            return response;
        }

        void FetchCommand::AppendFetchItem(std::string& response, const FetchItem& item, const Service::MailBoxService::ImapFetchMessage& message, const ParsedMailView* mailView, size_t rawSize) const {
            switch (item.type) {
                case FetchItemType::Flags:
                    response.append("FLAGS ");
                    response.append(BuildFlags(message));
                    return;

                case FetchItemType::Uid:
                    response.append("UID ");
                    response.append(std::to_string(message.imapUid));
                    return;

                case FetchItemType::Rfc822Size:
                    response.append("RFC822.SIZE ");
                    response.append(std::to_string(rawSize));
                    return;

                case FetchItemType::InternalDate:
                    response.append("INTERNALDATE \"");
                    response.append(FormatImapInternalDate(message.internalDate));
                    response.push_back('"');
                    return;

                case FetchItemType::Envelope:
                    response.append("ENVELOPE ");
                    response.append(BuildEnvelope(mailView));
                    return;

                case FetchItemType::BodyStructure:
                    response.append("BODYSTRUCTURE ");
                    response.append(BuildBodyStructure(mailView, rawSize));
                    return;

                case FetchItemType::BodyFull: {
                    const std::string& data = mailView ? mailView->raw : std::string{};
                    response.append(item.responseName.empty() ? "BODY[]" : item.responseName);
                    response.append(" {");
                    response.append(std::to_string(data.size()));
                    response.append("}\r\n");
                    response.append(data);
                    return;
                }

                case FetchItemType::BodyHeader: {
                    const std::string& data = mailView ? mailView->header : std::string{};
                    response.append(item.responseName.empty() ? "BODY[HEADER]" : item.responseName);
                    response.append(" {");
                    response.append(std::to_string(data.size()));
                    response.append("}\r\n");
                    response.append(data);
                    return;
                }

                case FetchItemType::BodyText: {
                    const std::string& data = mailView ? mailView->body : std::string{};
                    response.append(item.responseName.empty() ? "BODY[TEXT]" : item.responseName);
                    response.append(" {");
                    response.append(std::to_string(data.size()));
                    response.append("}\r\n");
                    response.append(data);
                    return;
                }

                case FetchItemType::BodyHeaderFields: {
                    std::string data;
                    if (mailView) data = ExtractHeaderFields(mailView->header, item.headerFields);

                    response.append("BODY[HEADER.FIELDS (");

                    for (size_t i = 0; i < item.headerFields.size(); ++i) {
                        if (i > 0) response.push_back(' ');
                        response.append(item.headerFields[i]);
                    }

                    response.append(")] {");
                    response.append(std::to_string(data.size()));
                    response.append("}\r\n");
                    response.append(data);
                    return;
                }
            }
        }

        std::string FetchCommand::BuildFlags(
            const Service::MailBoxService::ImapFetchMessage& message
        ) const {
            std::string flags = "(";
            bool first = true;

            auto add = [&](const char* flag) {
                if (!first) flags.push_back(' ');
                flags.append(flag);
                first = false;
            };

            if (message.isRead) add("\\Seen");
            if (message.isStarred) add("\\Flagged");
            if (message.isAnswered) add("\\Answered");

            flags.push_back(')');
            return flags;
        }

        std::string FetchCommand::BuildEnvelope(
            const ParsedMailView* mailView
        ) const {
            if (!mailView) return "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL)";

            const std::string date = GetHeaderValue(mailView->header, "Date");
            const std::string subject = GetHeaderValue(mailView->header, "Subject");
            const std::string from = GetHeaderValue(mailView->header, "From");
            const std::string to = GetHeaderValue(mailView->header, "To");
            const std::string cc = GetHeaderValue(mailView->header, "Cc");
            const std::string bcc = GetHeaderValue(mailView->header, "Bcc");
            const std::string replyTo = GetHeaderValue(mailView->header, "Reply-To");
            const std::string inReplyTo = GetHeaderValue(mailView->header, "In-Reply-To");
            const std::string messageId = GetHeaderValue(mailView->header, "Message-Id");

            const std::string fromList = BuildAddressList(from);
            const std::string replyToList = replyTo.empty() ? fromList : BuildAddressList(replyTo);

            std::string env;
            env.reserve(512);

            env.push_back('(');
            env.append(QuoteNString(date));
            env.push_back(' ');
            env.append(QuoteNString(subject));
            env.push_back(' ');
            env.append(fromList);
            env.push_back(' ');
            env.append(fromList);
            env.push_back(' ');
            env.append(replyToList);
            env.push_back(' ');
            env.append(BuildAddressList(to));
            env.push_back(' ');
            env.append(BuildAddressList(cc));
            env.push_back(' ');
            env.append(BuildAddressList(bcc));
            env.push_back(' ');
            env.append(QuoteNString(inReplyTo));
            env.push_back(' ');
            env.append(QuoteNString(messageId));
            env.push_back(')');

            return env;
        }

        std::string FetchCommand::BuildBodyStructure(
            const ParsedMailView* mailView,
            size_t rawSize
        ) const {
            const size_t bodySize = mailView ? mailView->body.size() : rawSize;
            const size_t lines = mailView ? CountLines(mailView->body) : 0;

            std::string out;
            out.reserve(128);

            out.append("(\"TEXT\" \"PLAIN\" (\"CHARSET\" \"UTF-8\") NIL NIL \"8BIT\" ");
            out.append(std::to_string(bodySize));
            out.push_back(' ');
            out.append(std::to_string(lines));
            out.append(" NIL NIL NIL NIL)");

            return out;
        }

        std::string FetchCommand::ExtractHeaderFields(
            const std::string& header,
            const std::vector<std::string>& fields
        ) const {
            if (header.empty() || fields.empty()) return "\r\n";

            std::set<std::string> wanted;
            for (const auto& field : fields) {
                wanted.insert(ToUpperAscii(field));
            }

            std::string output;

            size_t pos = 0;

            while (pos < header.size()) {
                size_t lineEnd = header.find('\n', pos);
                if (lineEnd == std::string::npos) lineEnd = header.size();
                else ++lineEnd;

                std::string block = header.substr(pos, lineEnd - pos);
                pos = lineEnd;

                while (pos < header.size() && (header[pos] == ' ' || header[pos] == '\t')) {
                    size_t contEnd = header.find('\n', pos);
                    if (contEnd == std::string::npos) contEnd = header.size();
                    else ++contEnd;

                    block.append(header.substr(pos, contEnd - pos));
                    pos = contEnd;
                }

                const size_t colon = block.find(':');
                if (colon == std::string::npos) continue;

                const std::string name = ToUpperAscii(TrimCopy(std::string_view(block).substr(0, colon)));

                if (wanted.find(name) != wanted.end()) {
                    output.append(block);
                    if (output.size() < 2 ||
                        output.substr(output.size() - 2) != "\r\n") {
                        if (!output.empty() && output.back() == '\n') {
                            // 已经有 LF，不额外处理。
                        } else {
                            output.append("\r\n");
                        }
                    }
                }
            }

            output.append("\r\n");
            return output;
        }

        std::string FetchCommand::GetHeaderValue(
            const std::string& header,
            std::string_view fieldName
        ) {
            const std::string wanted = ToUpperAscii(fieldName);

            size_t pos = 0;

            while (pos < header.size()) {
                size_t lineEnd = header.find('\n', pos);
                if (lineEnd == std::string::npos) lineEnd = header.size();
                else ++lineEnd;

                std::string block = header.substr(pos, lineEnd - pos);
                pos = lineEnd;

                while (pos < header.size() && (header[pos] == ' ' || header[pos] == '\t')) {
                    size_t contEnd = header.find('\n', pos);
                    if (contEnd == std::string::npos) contEnd = header.size();
                    else ++contEnd;

                    block.append(header.substr(pos, contEnd - pos));
                    pos = contEnd;
                }

                const size_t colon = block.find(':');
                if (colon == std::string::npos) continue;

                const std::string name = ToUpperAscii(TrimCopy(std::string_view(block).substr(0, colon)));

                if (name == wanted) {
                    return TrimCopy(std::string_view(block).substr(colon + 1));
                }
            }

            return {};
        }

        std::string FetchCommand::BuildAddressList(
            const std::string& value
        ) {
            if (TrimCopy(value).empty()) return "NIL";

            std::string output = "(";

            bool first = true;
            size_t begin = 0;

            for (;;) {
                size_t comma = value.find(',', begin);

                std::string item = TrimCopy(
                    std::string_view(value).substr(
                        begin,
                        comma == std::string::npos ? std::string::npos : comma - begin
                    )
                );

                std::string name;
                std::string email = item;

                const size_t lt = item.find('<');
                const size_t gt = item.find('>');

                if (lt != std::string::npos && gt != std::string::npos && gt > lt) {
                    name = TrimCopy(std::string_view(item).substr(0, lt));
                    email = TrimCopy(std::string_view(item).substr(lt + 1, gt - lt - 1));

                    if (name.size() >= 2 && name.front() == '"' && name.back() == '"') {
                        name = name.substr(1, name.size() - 2);
                    }
                }

                const size_t at = email.find('@');

                std::string mailbox;
                std::string host;

                if (at != std::string::npos) {
                    mailbox = email.substr(0, at);
                    host = email.substr(at + 1);
                } else {
                    mailbox = email;
                    host = "";
                }

                if (!mailbox.empty()) {
                    if (!first) output.push_back(' ');

                    output.push_back('(');
                    output.append(QuoteNString(name));
                    output.append(" NIL ");
                    output.append(QuoteNString(mailbox));
                    output.push_back(' ');
                    output.append(QuoteNString(host));
                    output.push_back(')');

                    first = false;
                }

                if (comma == std::string::npos) break;
                begin = comma + 1;
            }

            output.push_back(')');

            if (first) return "NIL";
            return output;
        }

        std::string FetchCommand::QuoteNString(std::string_view value) {
            if (value.empty()) return "NIL";
            return QuoteAString(value);
        }

        std::string FetchCommand::QuoteAString(std::string_view value) {
            std::string out;
            out.reserve(value.size() + 2);

            out.push_back('"');

            for (char ch : value) {
                if (ch == '\\' || ch == '"') out.push_back('\\');
                if (ch == '\r' || ch == '\n') {
                    out.push_back(' ');
                    continue;
                }

                out.push_back(ch);
            }

            out.push_back('"');
            return out;
        }

        std::string FetchCommand::ToUpperAscii(std::string_view value) {
            std::string out;
            out.reserve(value.size());

            for (char ch : value) {
                out.push_back(static_cast<char>(
                    std::toupper(static_cast<unsigned char>(ch))
                ));
            }

            return out;
        }

        std::string FetchCommand::TrimCopy(std::string_view value) {
            while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r' || value.front() == '\n')) {
                value.remove_prefix(1);
            }

            while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r' || value.back() == '\n')) {
                value.remove_suffix(1);
            }

            return std::string(value);
        }

        bool FetchCommand::EqualsIgnoreCase(
            std::string_view lhs,
            std::string_view rhs
        ) {
            return ToUpperAscii(lhs) == ToUpperAscii(rhs);
        }

        bool FetchCommand::StartsWithIgnoreCase(
            std::string_view value,
            std::string_view prefix
        ) {
            if (value.size() < prefix.size()) return false;
            return EqualsIgnoreCase(value.substr(0, prefix.size()), prefix);
        }

        bool FetchCommand::ContainsIgnoreCase(
            std::string_view value,
            std::string_view needle
        ) {
            return ToUpperAscii(value).find(ToUpperAscii(needle)) != std::string::npos;
        }

        size_t FetchCommand::CountLines(std::string_view value) {
            if (value.empty()) return 0;

            size_t lines = 0;

            for (char ch : value) {
                if (ch == '\n') ++lines;
            }

            if (value.back() != '\n') ++lines;

            return lines;
        }

    }
}