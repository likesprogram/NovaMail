#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <types/Types.hpp>
#include <LikesProgram/net/Buffer.hpp>
#include <service/EmailService.hpp>
#include <service/MailBoxService.hpp>

namespace NovaMail {
    namespace Smtp {
        struct StoreTask {
            uint64_t mailId;
            std::string bucket;
            std::string filePath;
            
            Types::MailFrom mailFrom;
            std::vector<Types::RcptTo> rcpTos;

            std::shared_ptr<LikesProgram::Net::Buffer> data;

            int retry = 0;
        };
        struct DeliverTask {
            uint64_t mailId;
            Types::MailFrom mailFrom;
            std::vector<Types::RcptTo> rcpTos;
            std::shared_ptr<LikesProgram::Net::Buffer> data;
            bool localTask = true;
            int retry = 0;
        };
        

        class MailDispatcher {
        public:
            // 获取单例
            static MailDispatcher& Instance();
            
            // 禁止拷贝
            MailDispatcher(const MailDispatcher&) = delete;
            MailDispatcher& operator=(const MailDispatcher&) = delete;

            // 禁止移动
            MailDispatcher(MailDispatcher&&) = delete;
            MailDispatcher& operator=(MailDispatcher&&) = delete;

            // 发送至外部邮箱
            bool DeliverOutboundDirectly(
                std::string smtpHostname, Service::EmailService& emailService, const uint64_t mailId,
                std::string fromMail, uint64_t fromMailId, bool isLocalSource, std::unordered_map<std::string, std::vector<std::string>> outboundRcpts
            );

            // 初始化
            static void Initialize(size_t storageThreads = 2, size_t deliverThreads = 2);
            
            // 获取 Smtp 服务名称
            std::string SmtpHostname() const noexcept;

            // 入队（存储）
            bool EnqueueStore(StoreTask task);

            // 入队（投递）
            bool EnqueueDeliver(DeliverTask task);

            // 当前负载（用于限流）
            size_t PendingStoreSize() const;
            size_t PendingDeliverSize() const;

            // 停止（优雅关闭）
            void Stop();

            // 析构
            ~MailDispatcher();
        private:
            // 构造函数私有化
            MailDispatcher(size_t storageThreads = 2, size_t deliverThreads = 2);
            void StorageWorker();   // 存储线程
            void DeliverWorker();   // 投递线程

            bool PopStoreTask(StoreTask& task);
            bool PopDeliverTask(DeliverTask& task);
        private:
            // 单例
            static std::unique_ptr<MailDispatcher> s_instance;
            static std::once_flag s_initFlag;
            
            // 线程
            std::vector<std::thread> m_storageThreads; // 存储线程
            std::vector<std::thread> m_deliverThreads; // 投递线程

            // 任务队列
            std::queue<StoreTask> m_storeQueue;         // 存储队列
            std::queue<DeliverTask> m_deliverQueue;     // 投递队列

            // 队列锁
            mutable std::mutex m_storeMutex;            // 存储锁
            mutable std::mutex m_deliverMutex;          // 投递锁

            // 条件变量
            std::condition_variable m_storeCv;          // 存储条件
            std::condition_variable m_deliverCv;        // 投递条件
            
            std::string m_smtpHostname = "NovaMail";    // SMTP 服务名称

            // 中继开关
            bool m_relayPolicy = false;

            // 状态
            std::atomic<bool> m_running{true};

            // 限量
            size_t m_maxQueueSize = 10000;

            // 邮件服务
            Service::EmailService m_emailService;
            Service::MailBoxService m_mailBoxService;

            // 最大重试次数
            size_t m_maxRetry = 3;
        };
    }
}
