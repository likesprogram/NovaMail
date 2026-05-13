#include <service/EmailService.hpp>
#include <command/DataCommand.hpp>
#include <MailDispatcher.hpp>
#include <Session.hpp>
#include <Transaction.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Smtp {
        void DataCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;
            (void) command;
            // 获取当前状态
            Types::State state = session->GetState();

            if (state == Types::State::Connected || state == Types::State::Greeted || state == Types::State::Identified || state == Types::State::MailStarted) {
                LogWarn(u"Received DATA command before EHLO/HELO, MAIL FROM or RCPT TO.");
                session->Reply(Types::ReplyCode::BadSequence, "Bad sequence of commands, send HELO/EHLO, MAIL FROM or RCPT TO first");
                return;
            }

            Transaction& transaction = session->GetTransaction();
            // 清空上一封邮件缓存
            transaction.ResetData();
            // 清除原有数据
            Reset();

            // 日志记录
            const Types::MailFrom& mailFrom = transaction.GetMailFrom();
            std::string mailFromAddress = mailFrom.HasAddress() ? mailFrom.AddressString() : "No Address";
            const std::vector<Types::RcptTo>& rcptTos = transaction.GetRcptTos();
            std::ostringstream oss;
            bool first = true;
            for (const auto& rcptTo : rcptTos) {
                if (!first) oss << ' ';
                oss << (rcptTo.HasAddress() ? rcptTo.AddressString() : "No Address");
                first = false;
            }
            std::string rcptToAddress = oss.str();
            LogInfo(
                u"DATA begin: from={} rcptCount={} recipients: {}",
                std::string(mailFromAddress),
                std::to_string(transaction.GetRcptCount()),
                rcptTos.empty() ? std::string("none") : rcptToAddress
            );
            
            session->SetState(Types::State::DataReceiving); // 切换状态
            session->Reply(Types::ReplyCode::StartMailInput, "End data with <CR><LF>.<CR><LF>"); // 发送响应
        }

        CommandHandler::DataResult DataCommand::InputData(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in) {
            size_t consumeSize = 0;
            Transaction& transaction = session->GetTransaction();

            CommandHandler::DataResult result = ExtractData(transaction, in.Peek(), in.ReadableBytes(), consumeSize);
            in.Consume(consumeSize); // 消费
            if (!session) return result;

            // 未接收完，继续接收
            if (result != CommandHandler::DataResult::TransactionFinished) return result;
        
            const size_t dataSize = transaction.DataBytes();
            const size_t overflowSize = transaction.GetDataOverflowSize();
            const size_t limit = transaction.MaxDataBytes();
            const std::string clientDomain = transaction.HasEhloDomain() ? std::string(transaction.EhloDomain()) : std::string("unknown");

            // 邮件大小超限
            if (transaction.DataOverflow())  {
                const size_t totalSize = limit + overflowSize;
                LogWarn(
                    u"Message size exceeds fixed maximum message size: size={} limit={} overflow={} clientDomain={}",
                    std::to_string(totalSize), std::to_string(limit), std::to_string(overflowSize), clientDomain
                );
                session->Reply(Types::ReplyCode::MessageTooLarge, "Message size exceeds fixed maximum message size");
                return result;
            }
            
            // 中继漏洞检测
            Types::MailFrom& mailFrom = transaction.GetMailFrom();
            if (mailFrom.IsExternalSource() || mailFrom.IsNullReversePath()) {
                for (const Types::RcptTo& rcptTo : transaction.GetRcptTos()) {
                    if (rcptTo.IsOutboundTarget()) {
                        LogWarn(
                            u"DATA command rejected: external source attempted outbound relay, mailFrom={} rcptTo={} client={} -> server={}",
                            mailFrom.Address().ToString(), rcptTo.Address().ToString(), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                        );
                        session->Reply(Types::ReplyCode::RelayDenied, "<" + rcptTo.Address().ToString() + ">: Relay access denied");
                        return result;
                    }
                }
            }

            // 正常邮件
            if (dataSize > 0 && transaction.Data().ReadableBytes() > 0) {
                uint64_t mailId;
                std::string filePath;
                static std::string storageBucket = "mails";

                try {
                    // 插入数据库
                    Service::EmailService emailService(LikeHub::Storage::DatabasePool::Instance());
                    if (!emailService.InsertMail(storageBucket, mailId, filePath, transaction.GetMailFrom(), transaction.Data())) {
                        LogError(
                            u"SMTP message rejected: failed to store metadata, from=<{}> rcptCount={} size={} client={} -> server={}",
                            transaction.GetMailFrom().Address().ToString(),
                            transaction.GetRcptTos().size(),
                            std::to_string(dataSize),
                            session->GetRemoteAddress().ToString(),
                            session->GetLocalAddress().ToString()
                        );
                        session->Reply(Types::ReplyCode::LocalError, "Failed to store mail metadata");
                        return result;
                    }

                    // 拷贝数据
                    auto buffer = std::make_shared<LikesProgram::Net::Buffer>();
                    buffer->Append(transaction.Data().Peek(), transaction.Data().ReadableBytes());

                    // 构造任务
                    StoreTask task;
                    task.mailId = mailId;
                    task.bucket = storageBucket;
                    task.filePath = filePath;
                    task.mailFrom = mailFrom;
                    task.rcpTos = transaction.GetRcptTos();
                    task.data = buffer;
                    task.retry = 0;

                    // 投递任务
                    if (!MailDispatcher::Instance().EnqueueStore(task)) {
                        LogError(u"SMTP message deferred: internal queue full, mailId={} from=<{}> rcptCount={} size={} client={} -> server={}",
                            std::to_string(mailId),
                            transaction.GetMailFrom().Address().ToString(),
                            transaction.GetRcptTos().size(),
                            std::to_string(dataSize),
                            session->GetRemoteAddress().ToString(),
                            session->GetLocalAddress().ToString()
                        );
                        emailService.MarkFailed(mailId); // 写入失败
                        session->Reply(Types::ReplyCode::InsufficientStorage, "Internal queue full");
                        return result;
                    }

                    LogInfo(
                        u"SMTP message received: mailId={} from=<{}> rcptCount={} size={} clientDomain={} client={} -> server={}",
                        std::to_string(mailId),
                        transaction.GetMailFrom().Address().ToString(),
                        transaction.GetRcptTos().size(),
                        std::to_string(dataSize),
                        clientDomain,
                        session->GetRemoteAddress().ToString(),
                        session->GetLocalAddress().ToString()
                    );

                    // 返回成功
                    session->Reply(Types::ReplyCode::Ok, "OK");
                    return result;
                } catch (const std::exception& e) {
                    LogError(
                        u"SMTP message failed: exception={}, from=<{}> rcptCount={} size={} client={} -> server={}",
                        e.what(),
                        transaction.GetMailFrom().Address().ToString(),
                        transaction.GetRcptTos().size(),
                        std::to_string(dataSize),
                        session->GetRemoteAddress().ToString(),
                        session->GetLocalAddress().ToString()
                    );
                    session->Reply(Types::ReplyCode::LocalError, "Internal server error");
                    return result;
                } catch (...) {
                    LogError(u"SMTP DATA processing unknown exception");
                    session->Reply(Types::ReplyCode::LocalError, "Internal server error");
                    return result;
                }
            }

            // DATA 空邮件，不做任何处理
            LogWarn(
                u"SMTP message finished but contains no data: clientDomain={}",
                clientDomain
            );
            session->Reply(Types::ReplyCode::Ok, "OK");
            return result;
        }

        void DataCommand::Reset() {
            m_dataComplete = false;
            m_dataTailLen = 0;
            std::memset(m_dataTail, 0, sizeof(m_dataTail));
        }

        bool DataCommand::FindDataEnd(const char* hold, size_t holdLen, const char* p, size_t len, size_t& matchEndInChunk, char newTail[4], size_t& newTailLen) {
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
                char tmp[8];
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

        CommandHandler::DataResult DataCommand::ExtractData(Transaction& transaction, const void* data, size_t len, size_t& consume) {
            consume = 0;
            if (len == 0) return DataResult::NeedMoreData;
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

            if (ended) { // 已找到结束标志
                const size_t streamEndPos = holdLen + matchEndInChunk;

                size_t contentLen = 0;
                if (streamEndPos >= m_dataEndLen) contentLen = streamEndPos - m_dataEndLen;

                const size_t commitHold = std::min(contentLen, holdLen);
                const size_t commitCur  = (contentLen > holdLen) ? (contentLen - holdLen) : 0;

                if (commitHold) transaction.AppendData(holdBuf, commitHold);
                if (commitCur) transaction.AppendData(p, commitCur);

                // 本包消耗到结束符末尾（上层可把剩余字节作为命令继续解析）
                consume = matchEndInChunk;

                // DATA 已结束，hold 清空
                m_dataTailLen = 0;

                return DataResult::TransactionFinished;
            }

            const size_t commitLen = (holdLen + len) - newTailLen;
            
            // 提交部分来自：holdBuf + chunk
            if (commitLen <= holdLen) {
                // 只提交 hold 的前 commitLen
                if (commitLen) transaction.AppendData(holdBuf, commitLen);
            } else {
                // 提交全部旧 hold
                if (holdLen) transaction.AppendData(holdBuf, holdLen);
                
                // 再提交本包前 (commitLen - holdLen) 字节
                const size_t commitCur = commitLen - holdLen;
                if (commitCur) transaction.AppendData(p, commitCur);
            }
            
            if (newTailLen) std::memcpy(m_dataTail, newTail, newTailLen);
            m_dataTailLen = newTailLen;

            consume = len;
            return DataResult::NeedMoreData;
        }
    }
}