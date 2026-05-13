#include <Transaction.hpp>

namespace NovaMail {
    namespace Smtp {
        Transaction::Transaction(size_t maxRcptCount, size_t maxDataBytes, bool dedupRcpt)
        :  m_mailFrom(Types::MailFrom()), m_maxRcptCount(maxRcptCount), m_maxDataBytes(maxDataBytes), m_dedupRcpt(dedupRcpt) {}
        
        void Transaction::ResetDomain() {
            // 清理 EHLO 参数
            m_ehloDomain = "unknown";
        }
        
        void Transaction::ResetList() {
            // 清理 列表
            m_mailFrom.Reset();
            m_rcptTos.clear();
            m_rcptSet.clear();

        }

        void Transaction::ResetData(bool restOverflow) {
            // 清理 文件数据
            m_data.RetrieveAll();
            m_dataBytes = 0;
            if (restOverflow) m_dataOverflowSize = 0;
        }

        bool Transaction::AddRcptTo(Types::RcptTo rcpt) {
            if (!rcpt.HasAddress()) return false; // 没有地址

            // 超限
            if (m_rcptTos.size() >= m_maxRcptCount) return false;

            if (m_dedupRcpt) {
                // 去重
                auto [it, inserted] = m_rcptSet.emplace(rcpt.AddressString());
                if (!inserted) return false; // duplicate
            }

            m_rcptTos.emplace_back(rcpt);
            return true;
        }

        void Transaction::AppendData(const void* data, size_t len) {
            if (DataOverflow()) {
                m_dataOverflowSize += len; // 记录溢出大小
                return; // 邮件大小溢出，拒绝写入
            }

            if (m_maxDataBytes > 0 && m_dataBytes + len > m_maxDataBytes) {
                m_dataOverflowSize = m_dataBytes + len - m_maxDataBytes;
                ResetData(false); // 重置数据，但不重置溢出标志
                return;
            }

            m_dataBytes += len;
            m_data.Append(data, len);
        }
    }
}