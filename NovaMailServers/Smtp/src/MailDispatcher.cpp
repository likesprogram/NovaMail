#include <MailDispatcher.hpp>
#include <MimeBuilder.hpp>
#include <LikeHub/DkimVerifier.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <LikeHub/LikeHubClient.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <MimeBuilder.hpp>
#include <LikeHub/SecurityContextManager.hpp>
#include <LikeHub/storage/fileSystem/FileSystem.hpp>
#include <smtpClient/SmtpClient.hpp>
#include <smtpClient/DnsResolver.hpp>
#include <iostream>
namespace {
    static std::string DotStuffForSmtpData(std::string_view rawMail) {
        /*
         * DKIM 注意：
         * 这里不要随意 Normalize 换行，否则可能破坏 DKIM。
         * 正确做法是：签名前就确保 MIME 邮件本身使用 CRLF。
         * 这里仅做 SMTP DATA 必需的 dot-stuffing。
         */
        std::string out;
        out.reserve(rawMail.size() + 32);

        bool lineStart = true;

        for (size_t i = 0; i < rawMail.size(); ++i) {
            char c = rawMail[i];

            if (lineStart && c == '.') {
                out.push_back('.');
            }

            out.push_back(c);

            if (c == '\n') {
                lineStart = true;
            } else {
                lineStart = false;
            }
        }

        /*
         * DATA 结束必须是 <CRLF>.<CRLF>。
         * 如果原始邮件没有以 CRLF 结束，这里补一个 CRLF。
         * 但商业级做法是：构造 MIME 邮件时就保证最后 CRLF 完整。
         */
        if (out.size() < 2 || out.substr(out.size() - 2) != "\r\n") {
            out += "\r\n";
        }

        out += ".\r\n";
        return out;
    }
}

namespace NovaMail {
    namespace Smtp {
        struct RecipientGroups {
            std::vector<NovaMail::Smtp::Types::RcptTo> localRcpts;
            std::vector<NovaMail::Smtp::Types::RcptTo> outboundRcpts;
            std::vector<NovaMail::Smtp::Types::RcptTo> unknownRcpts;
        };

        static void NewEmailDelivery(Service::MailBoxService::AddRecipientMappingsResult addRecipientMappings) {
            for (auto mailMapping : addRecipientMappings.mailMappings) {
                if (mailMapping.updated) continue;
                // 通知 智能分类模块 执行分类，若成功则由 智能分类模块通知邮件送达
                LikeHub::LikeHubClient& likeHub = LikeHub::LikeHubClient::Instance();
                LikesProgram::Configuration classificationMetadata;
                classificationMetadata[u"action"] = u"Classification";
                classificationMetadata[u"mappingId"] = LikesProgram::String(std::to_string(mailMapping.mappingId));
                classificationMetadata[u"mailBoxId"] = LikesProgram::String(std::to_string(mailMapping.mailBoxId));
                classificationMetadata[u"mailId"] = LikesProgram::String(std::to_string(mailMapping.mailId));
                classificationMetadata[u"folderId"] = LikesProgram::String(std::to_string(mailMapping.folderId));
                likeHub.SendMessage(LikeHub::RpcMessage::Encode(LikeHub::RpcMessage::Type::Request, 0, LikeHub::RpcMessage::Code::None, classificationMetadata, 0), 0);
            }
        }

        static RecipientGroups GroupRecipients(const std::vector<NovaMail::Smtp::Types::RcptTo>& rcpts) {
            RecipientGroups groups;
            for (const auto& rcpt : rcpts) {
                if (rcpt.IsLocalTarget()) groups.localRcpts.push_back(rcpt);
                else if (rcpt.IsOutboundTarget()) groups.outboundRcpts.push_back(rcpt);
                else groups.unknownRcpts.push_back(rcpt);
            }
            return groups;
        }
        
        bool MailDispatcher::DeliverOutboundDirectly(
            std::string smtpHostname, Service::EmailService& emailService, const uint64_t mailId,
            std::string fromMail, uint64_t fromMailId, bool isLocalSource, std::unordered_map<std::string, std::vector<std::string>> outboundRcpts
        ) {
            LogDebug(
                u"Direct outbound delivery started: mailId={} domainCount={}",
                static_cast<int64_t>(mailId), static_cast<int64_t>(outboundRcpts.size())
            );

            for (const auto& [domain, rcpts] : outboundRcpts) {
                LogDebug(u"Direct outbound delivery domain group: mailId={} domain={}", static_cast<int64_t>(mailId), domain);

                // 解析 MX
                NovaMail::Smtp::DnsResolver dnsResolver;
                auto mxRecords = dnsResolver.QueryMx(domain);
                if (mxRecords.empty()) {
                    LogWarn(u"MX test failed: domain={} reason=NoRecords", domain);
                } else {
                    LogInfo(u"MX test success: domain={} recordCount={}", domain, static_cast<int64_t>(mxRecords.size()));
                    for (const auto& mx : mxRecords) LogDebug(u"MX record: host={} preference={}", LikesProgram::String(mx.host), static_cast<int64_t>(mx.preference));
                }
                
                // 创建客户端
                NovaMail::Smtp::SmtpClient client(mxRecords[0].host, 25, smtpHostname, {}, std::chrono::milliseconds(10000));
                LikesProgram::Net::Buffer mailData;
                if (!emailService.GetMailData(mailId, mailData)) {
                    LogError(u"Classification failed: load mail data failed, mailId={}", std::to_string(mailId));
                    return false;
                }

                // 发送
                auto result = client.Deliver(fromMail, rcpts, DotStuffForSmtpData(std::string(reinterpret_cast<const char*>(mailData.Peek()), mailData.ReadableBytes())));
                if (!result.success) {
                    LogWarn(
                        u"Outbound SMTP delivery failed: mxHost={} code={} message={}",
                        LikesProgram::String(mxRecords[0].host), result.lastCode, LikesProgram::String(result.message)
                    );
                } else {
                    LogInfo(u"Outbound SMTP delivery success: mxHost={}", LikesProgram::String(mxRecords[0].host));
                }
            }
            
            if (isLocalSource) m_mailBoxService.UpdateSenderMailStatus(fromMailId, mailId, Service::MailBoxService::MailMappingStatus::Sent);
            return true;
        }

        std::unique_ptr<MailDispatcher> MailDispatcher::s_instance;
        std::once_flag MailDispatcher::s_initFlag;

        void MailDispatcher::Initialize(size_t storageThreads, size_t deliverThreads) {
            if (s_instance) LogWarn(u"MailDispatcher already initialized, reinitializing with new settings.");
            std::call_once(s_initFlag, [&](){
                LogInfo(u"Initializing MailDispatcher, storageThreads={}, deliverThreads={}", storageThreads, deliverThreads);
                s_instance.reset(new MailDispatcher(storageThreads, deliverThreads));
            });
        }

        MailDispatcher& MailDispatcher::Instance() {
            if (!s_instance) {
                LogFatal(u"MailDispatcher not initialized. Call Initialize() first.");
                throw std::runtime_error("MailDispatcher not initialized");
            }

            return *s_instance;
        }

        std::string MailDispatcher::SmtpHostname() const noexcept {
            return m_smtpHostname;
        }

        bool MailDispatcher::EnqueueStore(StoreTask task) {
            if (!m_running) return false;

            {
                std::lock_guard lock(m_storeMutex);

                if (m_storeQueue.size() >= m_maxQueueSize) {
                    LogWarn(u"Store queue full, rejecting task mailId={}", task.mailId);
                    return false;
                }

                m_storeQueue.push(std::move(task));
            }

            m_storeCv.notify_one();
            return true;
        }

        bool MailDispatcher::EnqueueDeliver(DeliverTask task) {
            if (!m_running) return false;

            {
                std::lock_guard lock(m_deliverMutex);
            
                if (m_deliverQueue.size() >= m_maxQueueSize) {
                    LogWarn(u"Deliver queue full, rejecting task mailId={}", task.mailId);
                    return false;
                }
            
                m_deliverQueue.push(std::move(task));
            }

            m_deliverCv.notify_one();
            return true;
        }

        size_t MailDispatcher::PendingStoreSize() const {
            std::lock_guard lock(m_storeMutex);
            return m_storeQueue.size();
        }

        size_t MailDispatcher::PendingDeliverSize() const {
            std::lock_guard lock(m_deliverMutex);
            return m_deliverQueue.size();
        }

        void MailDispatcher::Stop() {
            bool expected = true;
            if (!m_running.compare_exchange_strong(expected, false)) return; // 已经停止过，直接返回
        
            LogDebug(u"Stopping MailDispatcher...");
        
            // 唤醒所有 worker
            m_storeCv.notify_all();
            m_deliverCv.notify_all();
        
            // join storage threads
            for (auto& t : m_storageThreads) {
                if (t.joinable()) {
                    t.join();
                }
            }
        
            // join deliver threads
            for (auto& t : m_deliverThreads) {
                if (t.joinable()) {
                    t.join();
                }
            }
        
            LogDebug(u"MailDispatcher stopped.");
        }

        MailDispatcher::~MailDispatcher() {
            Stop(); // 统一走 Stop
        }

        MailDispatcher::MailDispatcher(size_t storageThreads, size_t deliverThreads)
        : m_running(true), m_emailService(LikeHub::Storage::DatabasePool::Instance()), 
        m_mailBoxService(LikeHub::Storage::DatabasePool::Instance()) {
            LogDebug(u"Mail dispatcher starting: storage worker threads={} delivery worker threads={}", storageThreads, deliverThreads);
            // 获取配置
            LikeHub::LikeHubConfig& config = LikeHub::LikeHubConfig::Instance();
            auto configRaw = config.Raw();
            // 是否开启中继
            if (configRaw[u"smtp"][u"relay_policy"].AsBool()) m_relayPolicy = true;
            // 获取服务名称
            m_smtpHostname = configRaw[u"smtp"][u"hostname"].AsString().ToStdString();
            // Storage Workers
            m_storageThreads.reserve(storageThreads);
            for (size_t i = 0; i < storageThreads; ++i) {
                m_storageThreads.emplace_back([this, i]() {
                    try {
                        LogDebug(u"Mail storage worker started: workerId={}", i);
                        this->StorageWorker();
                        LogDebug(u"Mail storage worker stopped: workerId={}", i);
                    } catch (const std::exception& e) {
                        LogError(u"Mail storage worker terminated unexpectedly: workerId={} error={}", i, e.what());
                    } catch (...) {
                        LogError(u"Mail storage worker terminated unexpectedly: workerId={} unknown error", i);
                    }
                });
            }
        
            // Deliver Workers
            m_deliverThreads.reserve(deliverThreads);
            for (size_t i = 0; i < deliverThreads; ++i) {
                m_deliverThreads.emplace_back([this, i]() {
                    try {
                        LogDebug(u"Mail delivery worker started: workerId={}", i);
                        this->DeliverWorker();
                        LogDebug(u"Mail delivery worker stopped: workerId={}", i);
                    } catch (const std::exception& e) {
                        LogError(u"Mail delivery worker terminated unexpectedly: workerId={} error={}", i, e.what());
                    } catch (...) {
                        LogError(u"Mail delivery worker terminated unexpectedly: workerId={} unknown error", i);
                    }
                });
            }
        }

        void MailDispatcher::StorageWorker() {
            while (true) {
                StoreTask task;
            
                if (!PopStoreTask(task)) return; // Stop 且无任务
            
                try {
                    LogDebug(u"Mail storage task started: mailId={}", task.mailId);
                
                    if (!m_emailService.TryMarkProcessing(task.mailId)) {
                        LogDebug(u"Mail storage task skipped: already claimed by another worker, mailId={}", task.mailId);
                        continue;
                    }

                    auto deliverData = std::make_shared<LikesProgram::Net::Buffer>();
                    const char* rawMailData = reinterpret_cast<const char*>((*task.data).Peek());
                    const size_t rawMailSize = (*task.data).ReadableBytes();

                    if (task.mailFrom.IsLocalSource()) {
                        Entity::MailBox mailBox = task.mailFrom.MailBox();
                        if (!mailBox.Valid()) {
                            m_mailBoxService.GetMailBox(task.mailFrom.Address(), mailBox);
                            task.mailFrom.SetMailBox(mailBox); // 保存方便后续使用
                        }
                        // 本地来源邮件：由本站负责 DKIM 签名
                        std::string domain = task.mailFrom.Address().GetDomain().Name();
                        std::string selector = "default";
                        std::string identity = task.mailFrom.Address().ToString();
                        std::string privateKeyPem = LikeHub::SecurityContextManager::Instance().GetDkimKey(domain)->privateKeyPem;
                        if (privateKeyPem.empty()) {
                            LogError(u"DKIM signing failed: private key not found, mailId={} domain={}", static_cast<int64_t>(task.mailId), domain);
                        } else {
                            try {
                                Mime::BuildOptions buildOptions;
                                buildOptions.mailId = task.mailId;
                                buildOptions.from = Mime::MailAddress(identity, std::string(task.mailFrom.DisplayName()));
                            
                                for (const auto& rcpt : task.rcpTos) {
                                    Mime::MailAddress addr(rcpt.Address().ToString(), std::string(rcpt.DisplayName()));
                                
                                    // TODO : 在 RcptTo 里加 recipientKind，再按类型放入 to/cc/bcc。
                                    buildOptions.to.push_back(addr);
                                }
                            
                                buildOptions.subject = "(No Subject)";
                                buildOptions.bodyKind = Mime::BodyKind::Auto;
                                buildOptions.messageIdDomain = task.mailFrom.Address().GetDomain().Name();
                                std::string canonicalMail = Mime::MimeBuilder::BuildStandardMail(*task.data, buildOptions);

                                LikeHub::DkimSignOptions opt;
                                opt.domain = domain;
                                opt.selector = selector;
                                opt.identity = identity;
                                opt.privateKeyPem = privateKeyPem;
                                // 对未 dot-stuffing 的真实邮件内容签名
                                std::string dkimHeader = LikeHub::DkimVerifier::Sign(canonicalMail, opt);
                                dkimHeader += "\r\n";
                            
                                // 加入 签名头
                                deliverData->Append(dkimHeader.data(), dkimHeader.size());
                                LogDebug(
                                    u"DKIM signing completed: mailId={} domain={} selector={} identity={}",
                                    static_cast<int64_t>(task.mailId), domain, selector, identity
                                );
                            } catch (const std::exception& e) {
                                LogError(
                                    u"DKIM signing exception: mailId={} domain={} error={}",
                                    static_cast<int64_t>(task.mailId), domain, e.what()
                                );
                            }
                        }

                        // 添加发件人关联
                        Service::MailBoxService::MailMappingCreateResult mailMapping = m_mailBoxService.AddSenderMailMapping(task.mailFrom.MailBox().mailBoxId, task.mailId, 3);
                        // 添加标签
                        if (mailMapping.mappingId > 0) m_emailService.AddLabel(mailMapping.mappingId, 1);
                    }
                    deliverData->Append(rawMailData, rawMailSize);

                    bool ok = m_emailService.StortMail(task.bucket, task.filePath, *deliverData);
                
                    if (ok) {
                        LogDebug(u"Mail storage completed: mailId={}", task.mailId);
                        m_emailService.MarkStored(task.mailId);

                        // 推入投递队列
                        EnqueueDeliver({task.mailId, task.mailFrom, task.rcpTos, deliverData, 0});
                    } else {
                        if (++task.retry < static_cast<int>(m_maxRetry)) {
                            LogWarn(u"Mail storage retry scheduled: mailId={} retry={}", task.mailId, task.retry);
                            EnqueueStore(std::move(task));
                        } else {
                            LogError(u"Mail storage failed: mailId={} after retries={}", task.mailId, task.retry);
                            m_emailService.MarkFailed(task.mailId);
                        }
                    }
                } catch (const std::exception& e) {
                    LogError(u"Mail storage exception: mailId={} after retries={} error={}", task.mailId, task.retry, e.what());

                    // 重新入队
                    if (++task.retry < static_cast<int>(m_maxRetry)) EnqueueStore(std::move(task));
                    else {
                        LogError(u"Mail storage failed after exception: mailId={} retries={}", task.mailId, task.retry);
                        m_emailService.MarkFailed(task.mailId);
                    }
                } catch (...) {
                    LogError(u"Mail storage unknown exception: mailId={} after retries={}", task.mailId, task.retry);

                    // 重新入队
                    if (++task.retry < static_cast<int>(m_maxRetry)) EnqueueStore(std::move(task));
                    else {
                        LogError(u"Mail storage failed after exception: mailId={} retries={}", task.mailId, task.retry);
                        m_emailService.MarkFailed(task.mailId);
                    }
                }
            }
        }

        void MailDispatcher::DeliverWorker() {
            while (true) {
                DeliverTask task;
                if (!PopDeliverTask(task)) return;
            
                try {
                    LogDebug(u"Mail delivery task started: mailId={} recipientCount={}", task.mailId, task.rcpTos.size());
                
                    // 收件人分组
                    RecipientGroups groups = GroupRecipients(task.rcpTos);
                    LogDebug(
                        u"Mail recipients grouped: mailId={} local={} outbound={} unknown={}",
                        static_cast<int64_t>(task.mailId), static_cast<int64_t>(groups.localRcpts.size()), static_cast<int64_t>(groups.outboundRcpts.size()), static_cast<int64_t>(groups.unknownRcpts.size())
                    );

                    if (!groups.unknownRcpts.empty()) { // 未标记去向，直接丢弃
                        for (const auto& rcpt : groups.unknownRcpts) {
                            LogError(
                                u"Mail delivery rejected: unknown recipient target type, mailId={} recipient={}",
                                task.mailId,
                                rcpt.Address().ToString()
                            );
                        }
                        // 策略拒绝，不重试
                        continue;
                    }

                    if (task.mailFrom.IsLocalSource()) { // 本地邮箱，需要更新状态
                        // 更新投递状态
                        if (groups.localRcpts.empty() && (groups.outboundRcpts.empty() || !task.mailFrom.IsLocalSource())) {
                            // 标记失败
                            m_mailBoxService.UpdateSenderMailStatus(task.mailFrom.MailBox().mailBoxId, task.mailId, Service::MailBoxService::MailMappingStatus::Failed);
                            continue;
                        }
                        // 发送中
                        m_mailBoxService.UpdateSenderMailStatus(task.mailFrom.MailBox().mailBoxId, task.mailId, Service::MailBoxService::MailMappingStatus::Sending);
                    }

                    bool localOk = true;
                    bool outboundOk = true;
                    bool sourceVerification = true;
                    if (!groups.localRcpts.empty()) {
                        if (!task.mailFrom.IsLocalSource()) { // 外地邮件，验证签名
                            sourceVerification = LikeHub::DkimVerifier::Verify(*task.data).status == LikeHub::DkimVerifyStatus::Pass;
                        }
                        std::vector<Service::MailBoxService::RecipientMapping> mappings;
                        mappings.reserve(groups.localRcpts.size());

                        for (const auto& rcpt : groups.localRcpts) {
                            Entity::MailBox mailBox = rcpt.MailBox();
                            if (!mailBox.Valid()) {
                                auto result = m_mailBoxService.GetMailBox(rcpt.Address(), mailBox);
                                if (result != Service::MailBoxService::GetMailBoxResult::Normal && result != Service::MailBoxService::GetMailBoxResult::NotOwned) {
                                    LogError(
                                        u"Local delivery failed: recipient mailbox invalid, mailId={} recipient={}",
                                        static_cast<int64_t>(task.mailId), rcpt.Address().ToString()
                                    );
                                    localOk = false;
                                    break;
                                }
                            }

                            Service::MailBoxService::RecipientMapping mapping;
                            mapping.mailBoxId = mailBox.mailBoxId;
                            mapping.mappingType = Service::MailBoxService::MailMappingType::To;

                            mapping.folderId = 2;
                            mapping.isRead = false;
                            mapping.isStarred = false;
                        
                            mappings.push_back(mapping);
                        }
                    
                        if (localOk && !mappings.empty()) {
                            auto addRecipientMappings = m_mailBoxService.AddRecipientMailMappings(task.mailId, mappings, 2);
                            if (!addRecipientMappings) {
                                LogError(
                                    u"Local delivery failed: mailId={} localRecipientCount={}",
                                    task.mailId,
                                    static_cast<int64_t>(mappings.size())
                                );
                                localOk = false;
                            } else {
                                LogDebug(
                                    u"Local delivery completed: mailId={} localRecipientCount={}",
                                    task.mailId,
                                    static_cast<int64_t>(mappings.size())
                                );
                                for (auto& mailMapping : addRecipientMappings.mailMappings) {
                                    // 添加标签
                                    if (mailMapping.mappingId > 0 && !sourceVerification) m_emailService.AddLabel(mailMapping.mailBoxId, 4);
                                }
                                NewEmailDelivery(addRecipientMappings); // 派发成功
                            }
                        }
                    }

                    if (!localOk) {
                        outboundOk = false;
                    } else if (!groups.outboundRcpts.empty()) { // 非本地邮箱直接忽略
                        // 出站投递安全兜底
                        if (!task.mailFrom.IsLocalSource()) {
                            for (const auto& rcpt : groups.outboundRcpts) {
                                LogWarn(
                                    u"Outbound relay denied: non-local sender attempted outbound delivery, mailId={} sender={} recipient={}",
                                    static_cast<int64_t>(task.mailId),
                                    task.mailFrom.IsNullReversePath() ? std::string("<>") : task.mailFrom.Address().ToString(),
                                    rcpt.Address().ToString()
                                );
                            }
                        
                            LogError(
                                u"Mail delivery permanently rejected: mailId={} reason=external_source_outbound_relay_denied", static_cast<int64_t>(task.mailId)
                            );
                        
                            // 策略拒绝，不重试
                            continue;
                        }
                        
                        // 未开启中继, 且是本地任务
                        if (!m_relayPolicy && task.localTask) {
                            LikeHub::LikeHubClient& likeHub = LikeHub::LikeHubClient::Instance();
                            LikesProgram::Configuration SmtpElayPolicyMetadata;
                            SmtpElayPolicyMetadata[u"action"] = u"SmtpElayPolicy"; // Smtp 中继任务
                            SmtpElayPolicyMetadata[u"taskData"][u"mailId"] = static_cast<int64_t>(task.mailId);
                            SmtpElayPolicyMetadata[u"taskData"][u"isLocalSource"] = task.mailFrom.IsLocalSource();
                            SmtpElayPolicyMetadata[u"taskData"][u"mailFrom"] = LikesProgram::String(std::to_string(task.mailId));
                            for (auto& rcpt : groups.outboundRcpts) {
                                LikesProgram::Configuration projRcpt;
                                projRcpt[u"domain"] = LikesProgram::String(rcpt.Address().GetDomain().Name());
                                projRcpt[u"mailBox"] = LikesProgram::String(rcpt.Address().ToString());
                                SmtpElayPolicyMetadata[u"taskData"][u"rcpTos"].Push_back(projRcpt);
                            }
                            likeHub.SendMessage(LikeHub::RpcMessage::Encode(LikeHub::RpcMessage::Type::Request, 0, LikeHub::RpcMessage::Code::None, SmtpElayPolicyMetadata, 0), 0);
                        }

                        LogDebug(
                            u"Outbound delivery scheduled: mailId={} outboundRecipientCount={}",
                            task.mailId,
                            static_cast<int64_t>(groups.outboundRcpts.size())
                        );
                    
                        // 分组
                        std::unordered_map<std::string, std::vector<std::string>> groupedRcpts;
                        for (const auto& rcpt : groups.outboundRcpts) groupedRcpts[rcpt.Address().GetDomain().Name()].push_back(rcpt.Address().ToString());
                        outboundOk = DeliverOutboundDirectly(
                            SmtpHostname(), m_emailService, task.mailId, task.mailFrom.Address().ToString(),
                            task.mailFrom.MailBox().mailBoxId, task.mailFrom.IsLocalSource(), groupedRcpts
                        );

                        if (!outboundOk) {
                            LogError(
                                u"Outbound delivery failed: mailId={} outboundRecipientCount={}",
                                task.mailId,
                                static_cast<int64_t>(groups.outboundRcpts.size())
                            );
                        }
                    }

                    bool ok = localOk && outboundOk;
                
                    if (ok) {
                        LogDebug(u"Mail delivery completed: mailId={} recipientCount={}", task.mailId, task.rcpTos.size());
                        if (groups.outboundRcpts.empty()) { // 发送完成
                            if (task.mailFrom.IsLocalSource())
                                m_mailBoxService.UpdateSenderMailStatus(task.mailFrom.MailBox().mailBoxId, task.mailId, Service::MailBoxService::MailMappingStatus::Sent);
                        }
                    } else {
                        if (++task.retry < static_cast<int>(m_maxRetry)) {
                            LogWarn(u"Mail delivery retry scheduled: mailId={} retry={}", task.mailId, task.retry);
                            EnqueueDeliver(std::move(task));
                        
                        } else {
                            LogError(u"Mail delivery failed: mailId={} after retries={}", task.mailId, task.retry);
                            if (task.mailFrom.IsLocalSource()) // 标记失败
                                m_mailBoxService.UpdateSenderMailStatus(task.mailFrom.MailBox().mailBoxId, task.mailId, Service::MailBoxService::MailMappingStatus::Failed);
                        }
                    }
                
                } catch (const std::exception& e) {
                    LogError(u"Mail delivery exception: mailId={} error={}", task.mailId, e.what());

                    // 重新入队
                    if (++task.retry < static_cast<int>(m_maxRetry)) EnqueueDeliver(std::move(task));
                    else {
                        LogError(u"Mail delivery failed after exception: mailId={} retries={}", task.mailId, task.retry);
                        if (task.mailFrom.IsLocalSource()) // 标记失败
                            m_mailBoxService.UpdateSenderMailStatus(task.mailFrom.MailBox().mailBoxId, task.mailId, Service::MailBoxService::MailMappingStatus::Failed);
                    }
                } catch (...) {
                    LogError(u"Mail delivery unknown exception: mailId={}", task.mailId);
                
                    if (++task.retry < static_cast<int>(m_maxRetry)) {
                        EnqueueDeliver(std::move(task));
                    } else {
                        LogError(u"Mail delivery failed after unknown error: mailId={} retries={}", task.mailId, task.retry);
                        if (task.mailFrom.IsLocalSource()) // 标记失败
                            m_mailBoxService.UpdateSenderMailStatus(task.mailFrom.MailBox().mailBoxId, task.mailId, Service::MailBoxService::MailMappingStatus::Failed);
                    }
                }
            }
        }

        bool MailDispatcher::PopStoreTask(StoreTask& task) {
            std::unique_lock lock(m_storeMutex);

            m_storeCv.wait(lock, [&] {return !m_running || !m_storeQueue.empty();});
        
            if (!m_running && m_storeQueue.empty()) return false;
        
            task = std::move(m_storeQueue.front());
            m_storeQueue.pop();
            return true;
        }

        bool MailDispatcher::PopDeliverTask(DeliverTask& task) {
            std::unique_lock lock(m_deliverMutex);

            m_deliverCv.wait(lock, [&] {return !m_running || !m_deliverQueue.empty();});
        
            if (!m_running && m_deliverQueue.empty()) return false;
        
            task = std::move(m_deliverQueue.front());
            m_deliverQueue.pop();
            return true;
        }
    }
}
