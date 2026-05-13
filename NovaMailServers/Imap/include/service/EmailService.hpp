#pragma once
#include <LikeHub/service/Service.hpp>
#include <LikesProgram/net/Buffer.hpp>
#include <entity/MailBox.hpp>
#include <service/MailFrom.hpp>
#include <vector>
#include <string>

namespace NovaMail {
    namespace Imap::Service {
        
        class EmailService : public LikeHub::IService {
        public:
            explicit EmailService(LikeHub::Storage::DatabasePool& pool);

            // 插入邮件信息
            bool InsertMail(std::string_view storageBucket, uint64_t& mailId, std::string& filePath, const Types::MailFrom& mailFrom, const LikesProgram::Net::Buffer& mailData);

            // 抢任务
            bool TryMarkProcessing(uint64_t mailId);
            // 成功
            bool MarkStored(uint64_t mailId);
            // 失败
            bool MarkFailed(uint64_t mailId);

            // 持久化存储邮件
            bool StortMail(std::string_view storageBucket, std::string_view filePath, const LikesProgram::Net::Buffer& mailData);

            // 获取邮件大小
            bool GetMailSize(uint64_t mailId, uint64_t& size);

            // 获取邮件内容
            bool GetMailData(uint64_t mailId, LikesProgram::Net::Buffer& mailData);

            // 添加标签
            bool AddLabel(uint64_t mailBoxMailId, uint64_t labelId);
        };
    }
}
