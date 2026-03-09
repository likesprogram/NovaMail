#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <cstddef>
#include <LikesProgram/net/Buffer.hpp>
#include <iostream>

namespace NovaMail {
    namespace Smtp {
        class Transaction {
        public:
            /// <summary>
            /// 构造函数
            /// </summary>
            /// <param name="maxRcptCount">最大 RCPT 数量</param>
            /// <param name="maxDataBytes">邮件内容允许的最大字节数</param>
            /// <param name="dedupRcpt">是否对 RCPT 地址进行去重</param>
            Transaction(size_t maxRcptCount = 1000, size_t maxDataBytes = (50 * 1024 * 1024), bool dedupRcpt = true);

            // 重置数据
            void Reset() { ResetList(); ResetData(); ResetTailData(); } // 全部
            void ResetList(); // 清理 EHLO 参数和 MAIL FROM / RCPT 列表
            void ResetData(); // 清理 邮件内容数据
            void ResetTailData(); // 清理 挎包数据

            // EHLO Domain
            bool HasEhloDomain() const noexcept { return !m_ehloDomain.empty(); }
            std::string_view EhloDomain() const noexcept { return m_ehloDomain; }
            void SetEhloDomain(std::string_view domain) { m_ehloDomain.assign(domain.data(), domain.size()); }

            // MAIL FROM
            bool HasMailFrom() const noexcept { return !m_mailFrom.empty(); }
            std::string_view MailFrom() const noexcept { return m_mailFrom; }
            void SetMailFrom(std::string_view from) { m_mailFrom.assign(from.data(), from.size()); }

            // RCPT TO
            bool HasRcpt() const noexcept { return !m_rcptTos.empty(); }
            std::size_t RcptCount() const noexcept { return m_rcptTos.size(); }
            const std::vector<std::string>& RcptTos() const noexcept { return m_rcptTos; }

            // 返回 false 表示：超限/重复/非法
            bool AddRcptTo(std::string_view rcpt);

            // 返回 false 表示超过最大大小（上层应回 552 或 452，并终止该事务）
            bool AppendData(const void* data, size_t len, size_t& consume);

            // 当前 数据 大小
            size_t DataBytes() const noexcept { return m_dataBytes; }

            // 读取 data
            const LikesProgram::Net::Buffer& Data() const noexcept { return m_data; }
            LikesProgram::Net::Buffer& Data() noexcept { return m_data; }

            // 数据是否接收完成
            bool IsDataComplete() const noexcept { return m_dataComplete; }

            // 获取最大 RCPT 数量
            size_t MaxRcptCount() const noexcept { return m_maxRcptCount; }

            // 获取最大 邮件容许的字节数
            size_t MaxDataBytes() const noexcept { return m_maxDataBytes; }
            
        private:
            std::string m_ehloDomain = "unknown"; // EHLO 参数（客户端域名）
            std::string m_mailFrom = std::string();
            std::vector<std::string> m_rcptTos = std::vector<std::string>();
            std::unordered_set<std::string> m_rcptSet = std::unordered_set<std::string>(); // 去重集合
            LikesProgram::Net::Buffer m_data = LikesProgram::Net::Buffer(); // Mail 文件数据

            std::size_t m_dataBytes = 0; // 当前数据大小
            bool m_dataComplete = false; // 是否结束

            size_t m_maxRcptCount   = 1000; // 最大 RCPT 数量
            size_t m_maxDataBytes   = 50 * 1024 * 1024; // 邮件内容允许的最大字节数
            bool   m_dedupRcpt      = true; // 是否对 RCPT 地址进行去重

            bool FindDataEnd(const char* hold, size_t holdLen, const char* p, size_t len, size_t& matchEndInChunk, char newTail[4], size_t& newTailLen);

            static constexpr char m_dataEnd[] = "\r\n.\r\n"; // Data 结尾
            static constexpr size_t  m_dataEndLen = sizeof(m_dataEnd) - 1;
            char m_dataTail[4] = {};      // 上一次数据末尾最多4字节
            size_t m_dataTailLen = 0;   // 0..4
        };
    }
}
