#pragma once
#include <command/CommandHandler.hpp>
#include <service/MailFolderService.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace NovaMail {
    namespace Imap {
        enum class Mode : uint8_t { List = 0, Lsub };
        class ListCommand final : public AuthenticatedCommandHandler {
        public:
            ListCommand(Mode mode = Mode::List): m_folderService(LikeHub::Storage::DatabasePool::Instance()), m_mode(mode) { }
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;

            InputResult InputData(
                const std::shared_ptr<Session>& session,
                LikesProgram::Net::Buffer& in
            ) override;

        private:
            enum class Stage : uint8_t {
                None = 0,

                // 正在读取 reference name 的 literal 内容
                ReadingReferenceLiteral,

                // reference literal 读完后，需要继续读取 mailbox pattern 所在行
                NeedPatternLine,

                // 正在读取 mailbox pattern 的 literal 内容
                ReadingPatternLiteral,

                // mailbox pattern literal 读完后，需要消费命令结束 CRLF
                NeedCommandEnd
            };

            enum class ParseResult : uint8_t {
                Ok = 0,
                NeedReferenceLiteral,
                NeedPatternLiteral,
                Bad
            };

            struct LiteralSpec {
                size_t size{0};
                bool nonSync{false};
            };

            struct ListArguments {
                bool subscribedOnly{false};
                bool returnSpecialUse{false};

                std::string referenceName;
                std::vector<std::string> mailboxPatterns;
            };

        private:
            static constexpr size_t kMaxListLiteralBytes = 4096;
            static constexpr size_t kMaxListLineBytes = 8192;
            static constexpr char kHierarchyDelimiter = '/';

        private:
            ParseResult ParseInitialArguments(
                const std::string& args,
                ListArguments& out,
                LiteralSpec& literal
            ) const;

            ParseResult ParsePatternLine(
                const std::string& line,
                LiteralSpec& literal
            );

            bool ParseLiteralSpec(
                const std::string& text,
                size_t& pos,
                LiteralSpec& literal
            ) const;

            bool ReadAString(
                const std::string& text,
                size_t& pos,
                std::string& out,
                LiteralSpec* literal
            ) const;

            bool ReadMailboxPatterns(
                const std::string& text,
                size_t& pos,
                std::vector<std::string>& patterns,
                LiteralSpec& literal,
                bool& needLiteral
            ) const;

            bool ParseOptionalSelectionOptions(
                const std::string& text,
                size_t& pos,
                ListArguments& out
            ) const;

            bool ParseOptionalReturnOptions(
                const std::string& text,
                size_t& pos,
                ListArguments& out
            ) const;

            bool ReadBalancedParentheses(
                const std::string& text,
                size_t& pos,
                std::string& out
            ) const;

            bool BeginLiteralRead(
                const std::shared_ptr<Session>& session,
                const std::string& tag,
                Stage stage,
                const LiteralSpec& literal
            );

            InputResult ReadLiteralBytes(
                const std::shared_ptr<Session>& session,
                LikesProgram::Net::Buffer& in
            );

            InputResult ParsePatternLineFromBuffer(
                const std::shared_ptr<Session>& session,
                LikesProgram::Net::Buffer& in
            );

            InputResult ConsumeCommandEndAndExecuteList(
                const std::shared_ptr<Session>& session,
                LikesProgram::Net::Buffer& in
            );

            void ExecuteList(
                const std::shared_ptr<Session>& session,
                const std::string& tag
            );

            std::string BuildCanonicalPattern(
                const std::string& referenceName,
                const std::string& mailboxPattern
            ) const;

            bool MatchListPattern(
                const std::string& mailboxName,
                const std::string& pattern
            ) const;

            bool MatchListPatternRecursive(
                std::string_view mailboxName,
                size_t namePos,
                std::string_view pattern,
                size_t patternPos
            ) const;

            std::string BuildListAttributes(
                const Service::MailFolderInfo& folder
            ) const;

            std::string QuoteString(const std::string& value) const;

            void SkipSpaces(const std::string& text, size_t& pos) const;

            static std::string ToUpperAscii(std::string_view value);
            static bool ContainsWordIgnoreCase(std::string_view text, std::string_view word);

            void ResetState();

        private:
            Service::MailFolderService m_folderService;
            Mode m_mode{Mode::List};
            Stage m_stage{Stage::None};

            std::string m_tag;
            ListArguments m_args;

            size_t m_expectedLiteralBytes{0};
            size_t m_receivedLiteralBytes{0};
            std::string m_literalBuffer;
        };
    }
}