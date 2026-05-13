#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <cstddef>
#include <LikesProgram/net/Buffer.hpp>
#include <iostream>
#include <types/Types.hpp>

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
            void Reset(bool restDomain = true) {
                if (restDomain) ResetDomain();
                ResetList(); ResetData();
            }
            void ResetDomain(); // 清理 EHLO Domain
            void ResetList(); // 清理 MAIL FROM / RCPT 列表
            void ResetData(bool restOverflow = true); // 清理 邮件内容数据

            // EHLO Domain
            bool HasEhloDomain() const noexcept { return !m_ehloDomain.empty(); }
            std::string_view EhloDomain() const noexcept { return m_ehloDomain; }
            void SetEhloDomain(std::string_view domain) { m_ehloDomain.assign(domain.data(), domain.size()); }

            // MAIL FROM
            const Types::MailFrom& GetMailFrom() const noexcept { return m_mailFrom; }
            Types::MailFrom& GetMailFrom() noexcept { return m_mailFrom; }

            // RCPT TO
            std::size_t GetRcptCount() const noexcept { return m_rcptTos.empty() ? 0 : m_rcptTos.size(); }
            const std::vector<Types::RcptTo>& GetRcptTos() const noexcept { return m_rcptTos; }

            // 返回 false 表示：超限/重复/非法
            bool AddRcptTo(Types::RcptTo rcpt);

            // 向内部缓冲区追加邮件数据。
            // 如果追加后的总数据大小超过配置的最大限制：
            //  1. 会设置数据溢出标志位；
            //  2. 本次追加操作会被拒绝；
            //  3. 后续所有 AppendData 调用也会被拒绝;
            // 数据溢出状态只能通过调用 ResetData(true) 进行清除。
            void AppendData(const void* data, size_t len);
            // 判断当前邮件数据是否已超过配置的最大大小限制。
            // 一旦返回 true，表示数据已发生溢出。
            // 在调用 ResetData(true) 之前将不再接受新的数据写入。
            bool DataOverflow() const noexcept { return m_dataOverflowSize > 0; }
            // 获取溢出大小
            size_t GetDataOverflowSize() const noexcept { return m_dataOverflowSize; }

            // 当前 数据 大小
            size_t DataBytes() const noexcept { return m_dataBytes; }

            // 读取 data
            const LikesProgram::Net::Buffer& Data() const noexcept { return m_data; }
            LikesProgram::Net::Buffer& Data() noexcept { return m_data; }

            // 获取最大 RCPT 数量
            size_t MaxRcptCount() const noexcept { return m_maxRcptCount; }

            // 获取最大 邮件容许的字节数
            size_t MaxDataBytes() const noexcept { return m_maxDataBytes; }
            
        private:
            std::string m_ehloDomain = "unknown"; // EHLO 参数（客户端域名）
            Types::MailFrom m_mailFrom; // MAIL FROM 相关信息
            std::vector<Types::RcptTo> m_rcptTos = std::vector<Types::RcptTo>();
            std::unordered_set<std::string> m_rcptSet = std::unordered_set<std::string>(); // 去重集合
            LikesProgram::Net::Buffer m_data = LikesProgram::Net::Buffer(); // Mail 文件数据

            std::size_t m_dataBytes = 0; // 当前数据大小

            size_t m_maxRcptCount   = 1000; // 最大 RCPT 数量
            size_t m_maxDataBytes   = 50 * 1024 * 1024; // 邮件内容允许的最大字节数
            bool   m_dedupRcpt      = true; // 是否对 RCPT 地址进行去重

            size_t m_dataOverflowSize = 0; // 数据大小超出限制
        };
    }
}
