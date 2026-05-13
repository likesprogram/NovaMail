#pragma once
#include <LikesProgram/net/Buffer.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <atomic>
#include <cstdint>

namespace NovaMail {
    namespace Smtp {
        class Command;
        class Session;
        class CommandHandler {
        public:
            // DATA / BDAT 数据接收阶段的处理结果
            enum class DataResult : uint8_t {
                // 当前缓冲区数据不足，尚未完成本次解析
                NeedMoreData = 0,
                // 当前 BDAT chunk 已接收完成，但邮件事务尚未结束，服务器应返回 250，并继续等待下一条 BDAT
                ChunkFinished = 1,
                // 整个邮件事务接收完成
                TransactionFinished = 2
            };

            virtual ~CommandHandler() = default;

            virtual void Handle(const std::shared_ptr<Session>& session, const Command& command) = 0;

            // 有数据处理的 指令处理类 需要重写数据写入函数 (主要是 DATA/BDAT 指令)
            virtual DataResult InputData(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in);
        };
    }
}