#include <Transaction.hpp>

namespace NovaMail {
    namespace Smtp {
        Transaction::Transaction(size_t maxRcptCount, size_t maxDataBytes, bool dedupRcpt) : m_maxRcptCount(maxRcptCount), m_maxDataBytes(maxDataBytes), m_dedupRcpt(dedupRcpt) {}
            
        void Transaction::ResetList() {
            // 清理 EHLO 参数
            m_ehloDomain = "unknown";

            // 清理 列表
            m_mailFrom.clear();
            m_rcptTos.clear();
            m_rcptSet.clear();

        }

        void Transaction::ResetData() {
            // 清理 文件数据
            m_data.RetrieveAll();
            m_dataBytes = 0;
            m_dataComplete = false;

        }

        void Transaction::ResetTailData() {
            // 清理 挎包数据
            m_dataTailLen = 0;
            std::memset(m_dataTail, 0, sizeof(m_dataTail));
        }

        bool Transaction::AddRcptTo(std::string_view rcpt) {
            if (rcpt.empty()) return false;
            // 超限
            if (m_rcptTos.size() >= m_maxRcptCount) return false;

            if (m_dedupRcpt) {
                // 去重
                auto [it, inserted] = m_rcptSet.emplace(rcpt);
                if (!inserted) return false; // duplicate
            }

            m_rcptTos.emplace_back(rcpt);
            return true;
        }

        bool Transaction::AppendData(const void* data, size_t len, size_t& consume) {
            consume = 0;
            if (len == 0) return true;
            if (m_dataComplete) return true;
            const char* p = static_cast<const char*>(data);

            // 保存 挎包数据
            char holdBuf[4];
            size_t holdLen = m_dataTailLen;           // 0..4
            if (holdLen) std::memcpy(holdBuf, m_dataTail, holdLen);

            // 寻找结束标识
            size_t matchEndInChunk = 0;
            char   newTail[4];
            size_t newTailLen = 0;
            const bool ended = FindDataEnd(holdBuf, holdLen, p, len, matchEndInChunk, newTail, newTailLen);

            if (ended) { // 已结束
                // matchEndInChunk：结束符末尾在本包中的位置（要消耗到这个位置）
                if (matchEndInChunk == 0 || matchEndInChunk > len) return false;

                const size_t streamEndPos = holdLen + matchEndInChunk;
                if (streamEndPos < m_dataEndLen) return false;  // 理论不可能：hold<=4, endLen=5

                const size_t contentLen = streamEndPos - m_dataEndLen; // 内容长度（在 hold+chunk 中）

                const size_t commitHold = std::min(contentLen, holdLen);
                const size_t commitCur  = (contentLen > holdLen) ? (contentLen - holdLen) : 0;

                // 限流：按内容计（不含结束符）
                if (m_dataBytes + commitHold + commitCur > m_maxDataBytes) return false;

                if (commitHold) m_data.Append(holdBuf, commitHold);
                if (commitCur)  m_data.Append(p, commitCur);

                m_dataBytes += (commitHold + commitCur);
                m_dataComplete = true;

                // 本包消耗到结束符末尾（上层可把剩余字节作为命令继续解析）
                consume = matchEndInChunk;

                // DATA 已结束，hold 清空
                m_dataTailLen = 0;

                return true;
            }

            const size_t commitLen = (holdLen + len) - newTailLen;
            if (m_dataBytes + commitLen > m_maxDataBytes) return false;
            
            // 提交部分来自：holdBuf + chunk
            if (commitLen <= holdLen) {
                // 只提交 hold 的前 commitLen
                if (commitLen) m_data.Append(holdBuf, commitLen);
            } else {
                // 提交全部旧 hold
                if (holdLen) m_data.Append(holdBuf, holdLen);
                
                // 再提交本包前 (commitLen - holdLen) 字节
                const size_t commitCur = commitLen - holdLen;
                if (commitCur) m_data.Append(p, commitCur);
            }
            
            if (newTailLen) std::memcpy(m_dataTail, newTail, newTailLen);
            m_dataTailLen = newTailLen;

            m_dataBytes += commitLen;
            consume = len;
            return true;
        }
    
        bool Transaction::FindDataEnd(const char* hold, size_t holdLen, const char* p, size_t len, size_t& matchEndInChunk, char newTail[4], size_t& newTailLen) {
            matchEndInChunk  = 0;
            
            const size_t total = holdLen + len;
            newTailLen = std::min<size_t>(4, total);
            if (newTailLen) {
                if (newTailLen <= len) {
                    std::memcpy(newTail, p + (len - newTailLen), newTailLen);
                } else {
                    // 末尾跨 hold+chunk
                    const size_t fromHold = newTailLen - len;
                    std::memcpy(newTail, hold + (holdLen - fromHold), fromHold);
                    std::memcpy(newTail + fromHold, p, len);
                }
            }

            if (len == 0) return false;

            // 跨包检查：hold + 本包前4字节
            if (holdLen > 0) {
                char tmp[9];
                std::memcpy(tmp, hold, holdLen);

                const size_t take = std::min<size_t>(len, 4);
                std::memcpy(tmp + holdLen, p, take);

                const size_t t = holdLen + take;

                for (size_t s = 0; s < holdLen; ++s) {
                    if (s + m_dataEndLen <= t && std::memcmp(tmp + s, m_dataEnd, m_dataEndLen) == 0) {
                        matchEndInChunk = (s + m_dataEndLen) - holdLen; // 1..4
                        return true;
                    }
                }
            }

            // 末尾 5 字节 O(1) 检查
            if (len >= m_dataEndLen) {
                const size_t i = len - m_dataEndLen;
                if (std::memcmp(p + i, m_dataEnd, m_dataEndLen) == 0) {
                    matchEndInChunk = len;
                    return true;
                }
            }

            // 反向按 '\n' 跳扫
            for (size_t j = len; j-- > 0; ) {
                if (p[j] != m_dataEnd[4]) continue;
                if (j < 4) continue;
                const size_t i = j - 4;
                if (std::memcmp(p + i, m_dataEnd, m_dataEndLen) == 0) {
                    matchEndInChunk = i + m_dataEndLen;
                    return true;
                }
            }
            return false;
        }
    }
}