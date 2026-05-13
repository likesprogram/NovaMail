#include <LikeHub/service/Service.hpp>
#include <LikesProgram/net/Buffer.hpp>
#include <MailBoxService.hpp>
#include <vector>
#include <string>

namespace NovaMail::Classification {
    class EmailService : public LikeHub::IService {
    public:
        explicit EmailService(LikeHub::Storage::DatabasePool& pool);

        // 获取邮件内容
        bool GetMailData(uint64_t mailId, LikesProgram::Net::Buffer& mailData);

        // 添加分类标签
        bool AddLabel(uint64_t mailId, uint64_t labelId);
    private:
        NovaMail::Imap::Service::MailBoxService m_mailBoxService;
    };
}