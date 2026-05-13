#pragma once
#include <LikesProgram/net/Buffer.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <atomic>
#include <cstdint>

namespace NovaMail {
    namespace Imap {
        class Session;
        class Command;
        // IMAP 多阶段输入处理结果
        enum class InputResult : uint8_t {
            NeedMoreData = 0,   // 当前 Buffer 数据不足，需要继续等待
            Continue = 1,       // 本阶段处理完成，但命令还没有整体结束
            Finished = 2,       // 当前命令完整结束
            Failed = 3          // 当前命令处理失败
        };
        class CommandHandler {
        public:
            virtual ~CommandHandler() = default;

            virtual bool IsAllowed(const Session& session, const Command& command) const = 0;

            // 普通指令输入
            virtual void Handle(const std::shared_ptr<Session>& session, const Command& command) = 0;

            // 多阶段 “按行输入”
            virtual InputResult InputLine(const std::shared_ptr<Session>& session, const Command& line);

            // 多阶段 “按字节输入”
            virtual InputResult InputData(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in);
        };
        class AnyStateCommandHandler : public CommandHandler {
        public:
            bool IsAllowed(const Session& session, const Command& command) const override;
        };
        class NotAuthenticatedCommandHandler : public CommandHandler {
        public:
            bool IsAllowed(const Session& session, const Command& command) const override;
        };
        class AuthenticatedCommandHandler : public CommandHandler {
        public:
            bool IsAllowed(const Session& session, const Command& command) const override;
        };
        class SelectedCommandHandler : public CommandHandler {
        public:
            bool IsAllowed(const Session& session, const Command& command) const override;
        };
    }
}