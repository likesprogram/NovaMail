#pragma once
#include <LikeHub/service/Service.hpp>
#include <address/EmailAddress.hpp>
#include <entity/MailBox.hpp>
#include <string>

namespace NovaMail {
    namespace Smtp::Service {
        class MailBoxService : public LikeHub::IService {
        public:
            enum class GetMailBoxResult : uint8_t {
                NotFound  = 0,   // 邮箱不存在（查询未命中 / 未创建）
                Disabled  = 1,   // 邮箱被禁用（封禁 / 未激活 / 冻结）
                Normal    = 2,   // 正常可用
                NotOwned  = 3,   // 存在，但不属于当前用户
                Error     = 4    // 查询错误
            };

            enum class MailMappingType : uint8_t {
                From = 0,
                To   = 1
            };

            enum class MailMappingStatus : uint8_t {
                Normal    = 0,
                Queued    = 1,
                Sending   = 2,
                Sent      = 3,
                Failed    = 4
            };

            struct RecipientMapping {
                uint64_t mailBoxId = 0;
                MailMappingType mappingType = MailMappingType::To;

                // 0 表示使用默认收件箱
                uint64_t folderId = 0;

                bool isRead = false;
                bool isStarred = false;
            };
            struct MailMappingCreateResult {
                bool success{false};

                uint64_t mappingId{0};
                uint64_t mailBoxId{0};
                uint64_t mailId{0};
                uint64_t folderId{0};

                // IMAP 对外 UID
                uint64_t imapUid{0};

                bool created{false};      // true 表示新插入
                bool updated{false};      // true 表示更新已有映射

                explicit operator bool() const noexcept {
                    return success;
                }
            };
            struct AddRecipientMappingsResult {
                bool success = false;
                std::vector<MailMappingCreateResult> mailMappings;
                        
                explicit operator bool() const {
                    return success;
                }
            };

            explicit MailBoxService(LikeHub::Storage::DatabasePool& pool);

            // 获取邮箱
            GetMailBoxResult GetMailBox(const Address::EmailAddress& emailAddress, Entity::MailBox& mailbox, uint64_t loginUserId = 0);
            // 判断是否属于本地邮箱
            bool IsLocalDomain(const Address::Domain& domain);

            // 发件人与邮件关联：用于站内用户发信。后续会更新发送状态。
            MailMappingCreateResult AddSenderMailMapping(uint64_t mailBoxId, uint64_t mailId, uint64_t folderId, MailMappingStatus status = MailMappingStatus::Queued);

            // 更新发件人的投递状态：QUEUED / SENDING / SENT / FAILED
            bool UpdateSenderMailStatus(uint64_t mailBoxId, uint64_t mailId, MailMappingStatus status);

            // 收件人与邮件关联：本地投递完成时一次性批量写入。
            AddRecipientMappingsResult AddRecipientMailMappings(uint64_t mailId, const std::vector<RecipientMapping>& recipients, uint64_t defaultInboxFolderId);
        };
    }
}
