#pragma once
#include <command/CommandHandler.hpp>
#include <Transaction.hpp>
#include <types/Types.hpp>
#include <string>

namespace NovaMail {
    namespace Smtp {
        class DataCommand: public CommandHandler {
        public:
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;

            // 数据写入接口 (已重写)
            DataResult InputData(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in) override;
        private:
            // 重置数据
            void Reset();
            // 提取数据
            DataResult ExtractData(Transaction& transaction, const void* data, size_t len, size_t& consume);
            // 找到结尾标识符
            bool FindDataEnd(const char* hold, size_t holdLen, const char* p, size_t len, size_t& matchEndInChunk, char newTail[4], size_t& newTailLen);

            // 结尾判定
            static constexpr char m_dataEnd[] = "\r\n.\r\n"; // Data 结尾
            static constexpr size_t  m_dataEndLen = sizeof(m_dataEnd) - 1;

            // 挎包数据
            char m_dataTail[4] = {};      // 上一次数据末尾最多4字节
            size_t m_dataTailLen = 0;   // 0..4

            // 数据接收完成
            bool m_dataComplete = false;
        };
    }
}