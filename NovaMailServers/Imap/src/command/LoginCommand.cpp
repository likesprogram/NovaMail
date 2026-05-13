#include <command/LoginCommand.hpp>

#include <Session.hpp>
#include <command/Command.hpp>
#include <address/EmailAddress.hpp>
#include <LikesProgram/log/Logger.hpp>

#include <cctype>

namespace NovaMail {
    namespace Imap {
        void LoginCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;

            const std::string tag = std::string(command.Tag());
            const std::string args = std::string(command.Arguments());

            // STARTTLS 前不要允许 LOGIN
            if (!session->IsTLS()) {
                session->ReplyTagged(tag, Status::No, "LOGIN requires TLS");
                return;
            }

            // 已经登录过了
            if (session->Authenticated()) {
                session->ReplyTagged(tag, Status::Bad, "Already authenticated");
                return;
            }

            size_t pos = 0;
            std::string account;
            size_t literalSize = 0;
            bool nonSyncLiteral = false;

            auto accountResult = ReadLoginStringFromLine(args, pos, account, literalSize, nonSyncLiteral);

            if (accountResult == ReadStringResult::Bad || accountResult == ReadStringResult::NeedMoreData) {
                session->ReplyTagged(tag, Status::Bad, "Bad LOGIN account argument");
                return;
            }

            if (accountResult == ReadStringResult::NeedLiteral) {
                m_stage = Stage::ReadingAccountLiteral;
                BeginLiteralRead(session, tag, Stage::ReadingAccountLiteral, literalSize, nonSyncLiteral);
                return;
            }

            m_account = std::move(account);
            m_stage = Stage::NeedPassword;

            std::string password;
            literalSize = 0;
            nonSyncLiteral = false;

            auto passwordResult = ReadLoginStringFromLine(args, pos, password, literalSize, nonSyncLiteral
            );
        
            if (passwordResult == ReadStringResult::Bad || passwordResult == ReadStringResult::NeedMoreData) {
                session->ReplyTagged(tag, Status::Bad, "Bad LOGIN password argument");
                ResetLoginState();
                return;
            }
        
            if (passwordResult == ReadStringResult::NeedLiteral) {
                BeginLiteralRead(session, tag, Stage::ReadingPasswordLiteral, literalSize, nonSyncLiteral);
                return;
            }
        
            m_password = std::move(password);
        
            SkipSpaces(args, pos);
            if (pos != args.size()) {
                session->ReplyTagged(tag, Status::Bad, "Too many LOGIN arguments");
                ResetLoginState();
                return;
            }

            TryFinishLogin(session, tag);
        }
        
        InputResult LoginCommand::InputData(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in) {
            if (!session) return InputResult::Failed;
        
            for (;;) {
                if (m_stage == Stage::ReadingAccountLiteral || m_stage == Stage::ReadingPasswordLiteral) {
                    auto result = ReadLiteralBytes(session, in);
                    if (result == InputResult::NeedMoreData || result == InputResult::Failed || result == InputResult::Finished) return result;
                    continue;
                }
                if (m_stage == Stage::NeedPassword) return ParsePasswordFromBuffer(session, in);
                if (m_stage == Stage::NeedCommandEnd) return ConsumeCommandEndAndFinishLogin(session, in);
                return InputResult::Failed;
            }
        }

        InputResult LoginCommand::ConsumeCommandEndAndFinishLogin(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in) {
            if (!session) return InputResult::Failed;
        
            const size_t readable = in.ReadableBytes();
            if (readable == 0) return InputResult::NeedMoreData;
        
            const auto* data = in.Peek();
        
            size_t pos = 0;
        
            // LOGIN password literal 后理论上只允许空白和命令结束符
            while (pos < readable && (data[pos] == ' ' || data[pos] == '\t')) ++pos;
            if (pos >= readable) return InputResult::NeedMoreData;
            const std::string tag = std::string(session->ActiveTag());
            if (data[pos] == '\r') {
                if (pos + 1 >= readable) return InputResult::NeedMoreData;
                if (data[pos + 1] != '\n') {
                    session->ReplyTagged(tag, Status::Bad, "Bad LOGIN command ending");
                    ResetLoginState();
                    return InputResult::Finished;
                }
                in.Consume(pos + 2);
            } else if (data[pos] == '\n') {
                // 宽松兼容裸 LF
                in.Consume(pos + 1);
            } else {
                // password literal 后面还有非空白内容，属于多余参数
                session->ReplyTagged(tag, Status::Bad, "Too many LOGIN arguments");
                ResetLoginState();
                return InputResult::Finished;
            }
        
            m_stage = Stage::None;
            TryFinishLogin(session, tag);
            return InputResult::Finished;
        }

        void LoginCommand::ResetLoginState() {
            m_stage = Stage::None;
            m_account.clear();
            m_password.clear();

            m_expectedLiteralBytes = 0;
            m_receivedLiteralBytes = 0;
            m_literalBuffer.clear();
        }

        void LoginCommand::SkipSpaces(const std::string& text, size_t& pos) const {
            while (pos < text.size()) {
                const unsigned char ch = static_cast<unsigned char>(text[pos]);
                if (ch != ' ' && ch != '\t') break;
                ++pos;
            }
        }

        bool LoginCommand::ParseLiteralSpec(const std::string& text, size_t& pos, size_t& literalSize, bool& nonSyncLiteral) const {
            literalSize = 0;
            nonSyncLiteral = false;
            SkipSpaces(text, pos);

            if (pos >= text.size() || text[pos] != '{') return false;
            ++pos;

            const size_t numberBegin = pos;
            while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) ++pos;
            if (pos == numberBegin) return false;

            const size_t numberEnd = pos;
            if (pos < text.size() && text[pos] == '+') {
                nonSyncLiteral = true;
                ++pos;
            }

            if (pos >= text.size() || text[pos] != '}') return false;
            ++pos;

            try {
                literalSize = static_cast<size_t>(std::stoull(text.substr(numberBegin, numberEnd - numberBegin)));
            } catch (...) {
                return false;
            }

            SkipSpaces(text, pos);

            // 要求 literal marker 位于当前行末尾
            return pos == text.size();
        }

        LoginCommand::ReadStringResult LoginCommand::ReadLoginStringFromLine(const std::string& text, size_t& pos, std::string& out, size_t& literalSize, bool& nonSyncLiteral) {
            out.clear();
            literalSize = 0;
            nonSyncLiteral = false;

            SkipSpaces(text, pos);
            if (pos >= text.size()) return ReadStringResult::NeedMoreData;

            if (text[pos] == '{') {
                if (!ParseLiteralSpec(text, pos, literalSize, nonSyncLiteral)) return ReadStringResult::Bad;
                return ReadStringResult::NeedLiteral;
            }

            if (text[pos] == '"') {
                ++pos;
                while (pos < text.size()) {
                    char ch = text[pos++];
                    if (ch == '"') return ReadStringResult::Ok;
                    if (ch == '\\') {
                        if (pos >= text.size()) return ReadStringResult::Bad;
                        out.push_back(text[pos++]);
                        continue;
                    }
                    const unsigned char uch = static_cast<unsigned char>(ch);
                    if (uch == '\r' || uch == '\n')  return ReadStringResult::Bad;
                    out.push_back(ch);
                }
                return ReadStringResult::Bad;
            }

            while (pos < text.size()) {
                char ch = text[pos];
                if (ch == ' ' || ch == '\t') break;
                const unsigned char uch = static_cast<unsigned char>(ch);
                if (uch < 0x20 || uch == 0x7F) return ReadStringResult::Bad;
                if (ch == '(' || ch == ')' || ch == '{' || ch == '"' || ch == '\\') return ReadStringResult::Bad;
                out.push_back(ch);
                ++pos;
            }

            return out.empty() ? ReadStringResult::Bad : ReadStringResult::Ok;
        }

        bool LoginCommand::BeginLiteralRead(const std::shared_ptr<Session>& session, const std::string& tag, Stage literalStage, size_t literalSize, bool nonSyncLiteral) {
            if (literalSize > kMaxLoginLiteralBytes) {
                session->ReplyTagged(tag, Status::Bad, "LOGIN literal too large");
                ResetLoginState();
                return false;
            }

            if (nonSyncLiteral && literalSize > kMaxNonSyncLiteralBytes) {
                session->ReplyTagged(tag, Status::Bad, "Non-synchronizing literal too large");
                ResetLoginState();
                return false;
            }

            m_stage = literalStage;
            m_expectedLiteralBytes = literalSize;
            m_receivedLiteralBytes = 0;
            m_literalBuffer.clear();
            m_literalBuffer.reserve(literalSize);

            if (!nonSyncLiteral) session->ReplyContinuation("Ready for literal data");

            if (literalSize == 0) {
                if (literalStage == Stage::ReadingAccountLiteral) {
                    m_account.clear();
                    m_stage = Stage::NeedPassword;
                } else {
                    m_password.clear();
                    // 即使 literal 内容长度为 0，后面仍然还有命令结束 CRLF
                    m_stage = Stage::NeedCommandEnd;
                }
                return true;
            }

            // 进入 LiteralBytes
            session->BeginLiteralBytes(CommandVerb::LOGIN, tag);
            return true;
        }

        InputResult LoginCommand::ReadLiteralBytes(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in) {
            const size_t remaining = m_expectedLiteralBytes - m_receivedLiteralBytes;
            if (remaining == 0) return InputResult::Failed;
        
            const size_t readable = in.ReadableBytes();
            if (readable == 0) return InputResult::NeedMoreData;
        
            const size_t take = std::min(readable, remaining);
        
            m_literalBuffer.append(reinterpret_cast<const char*>(in.Peek()), take);
        
            in.Consume(take);
            m_receivedLiteralBytes += take;
        
            if (m_receivedLiteralBytes < m_expectedLiteralBytes) return InputResult::NeedMoreData;
        
            const std::string tag = std::string(session->ActiveTag());
        
            if (m_stage == Stage::ReadingAccountLiteral) {
                m_account = std::move(m_literalBuffer);
                m_literalBuffer.clear();
            
                if (m_account.empty()) {
                    session->ReplyTagged(tag, Status::Bad, "Empty LOGIN account");
                    ResetLoginState();
                    return InputResult::Finished;
                }
            
                m_stage = Stage::NeedPassword;
                return InputResult::Continue;
            }
        
            if (m_stage == Stage::ReadingPasswordLiteral) {
                m_password = std::move(m_literalBuffer);
                m_literalBuffer.clear();
                m_stage = Stage::NeedCommandEnd;
                return InputResult::Continue;
            }

            return InputResult::Failed;
        }

        InputResult LoginCommand::ParsePasswordFromBuffer(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in) {
            const size_t readable = in.ReadableBytes();
            if (readable == 0) return InputResult::NeedMoreData;

            const auto* data = in.Peek();
            size_t lfPos = static_cast<size_t>(-1);

            for (size_t i = 0; i < readable; ++i) {
                if (data[i] == '\n') {
                    lfPos = i;
                    break;
                }
            }

            if (lfPos == static_cast<size_t>(-1)) {
                if (readable > kMaxLoginLiteralBytes) {
                    session->ReplyTagged(std::string(session->ActiveTag()), Status::Bad, "LOGIN continuation too large");
                    ResetLoginState();
                    return InputResult::Finished;
                }
                return InputResult::NeedMoreData;
            }

            size_t lineLen = lfPos;
            if (lineLen > 0 && data[lineLen - 1] == '\r') --lineLen;

            std::string line(reinterpret_cast<const char*>(data), lineLen);
            in.Consume(lfPos + 1);

            size_t pos = 0;
            std::string password;
            size_t literalSize = 0;
            bool nonSyncLiteral = false;

            auto result = ReadLoginStringFromLine(line, pos, password, literalSize, nonSyncLiteral);
            const std::string tag = std::string(session->ActiveTag());

            if (result == ReadStringResult::Bad ||
                result == ReadStringResult::NeedMoreData) {
                session->ReplyTagged(tag, Status::Bad, "Bad LOGIN password argument");
                ResetLoginState();
                return InputResult::Finished;
            }

            if (result == ReadStringResult::NeedLiteral) {
                BeginLiteralRead(session, tag, Stage::ReadingPasswordLiteral, literalSize, nonSyncLiteral);
                return InputResult::Continue;
            }

            m_password = std::move(password);
            SkipSpaces(line, pos);

            if (pos != line.size()) {
                session->ReplyTagged(tag, Status::Bad, "Too many LOGIN arguments");
                ResetLoginState();
                return InputResult::Finished;
            }

            m_stage = Stage::None;
            TryFinishLogin(session, tag);
            return InputResult::Finished;
        }

        bool LoginCommand::TryFinishLogin(const std::shared_ptr<Session>& session, const std::string& tag) {
            if (m_account.empty()) {
                session->ReplyTagged(tag, Status::Bad, "Empty LOGIN account");
                return false;
            }
            const std::string account = m_account;
            const std::string password = m_password;

            ResetLoginState();
            return VerifyAndLogin(session, tag, account, password);
        }

        bool LoginCommand::VerifyAndLogin(const std::shared_ptr<Session>& session, const std::string& tag, const std::string& account, const std::string& password) {
            LogInfo(
                u"IMAP LOGIN attempt: user={}, client={} -> server={}",
                LikesProgram::String(account), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
            );

            Entity::User user;
            bool success = false;

            Address::EmailAddress address;
            // 邮箱地址登录
            if (Address::EmailAddress::Parse(account, address))
                success = m_userService.GetUserByMailboxAuthCode(address.Local(), address.GetDomain().Name(), password, user);
            // 普通账号登录
            else success = m_userService.GetUserByAccountAuthCode(account, password, user);

            if (!success || !user.Valid()) {
                LogWarn(
                    u"IMAP LOGIN failed: user={}, client={}",
                    LikesProgram::String(account), session->GetRemoteAddress().ToString()
                );

                session->ReplyTagged(tag, Status::No, "Authentication failed");
                return false;
            }

            if (!user.HasMailBox()) {
                LogWarn(
                    u"IMAP LOGIN failed: user={} userId={} reason=NoAvailableMailbox",
                    LikesProgram::String(account), static_cast<int64_t>(user.userId)
                );
            
                session->ReplyTagged(tag, Status::No, "No available mailbox");
                return false;
            }

            session->SetUser(user);

            LogInfo(u"IMAP LOGIN success: user={}, client={}", LikesProgram::String(account), session->GetRemoteAddress().ToString());
            session->ReplyTagged(tag, Status::Ok, "LOGIN completed");
            return true;
        }

    }
}