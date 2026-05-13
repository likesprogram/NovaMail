#pragma once
#include <LikeHub/service/Service.hpp>
#include <address/EmailAddress.hpp>
#include <MailFolderService.hpp>
#include <string>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace NovaMail {
    namespace Imap::Service {
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

            struct ImapMailboxCounter {
                bool success{false};

                // 当前邮箱文件夹邮件数量
                uint64_t exists{0};

                // 第一封未读邮件的 sequence number，不是未读数量
                uint64_t firstUnseenSeq{0};
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

            struct ImapFetchMessage {
                bool success{false};

                uint64_t mappingId{0};
                uint64_t mailBoxId{0};
                uint64_t folderId{0};
                uint64_t mailId{0};

                // IMAP UID，当前 mailBoxId + folderId 内唯一
                uint64_t imapUid{0};

                // 当前 mailbox 下的 sequence number
                uint64_t sequence{0};

                bool isRead{false};
                bool isStarred{false};
                bool isAnswered{false};

                // 邮件进入该用户该文件夹的时间
                std::time_t internalDate;
            };

            struct MoveMailResult {
                bool success{false};
                bool moved{false};
                bool merged{false};

                uint64_t mappingId{0};
                uint64_t targetMappingId{0};

                uint64_t mailBoxId{0};
                uint64_t mailId{0};

                uint64_t oldFolderId{0};
                uint64_t newFolderId{0};

                uint64_t oldImapUid{0};
                uint64_t newImapUid{0};
            };

            struct AddRecipientMappingsResult {
                bool success = false;
                std::vector<MailMappingCreateResult> mailMappings;
                        
                explicit operator bool() const {
                    return success;
                }
            };

            explicit MailBoxService(LikeHub::Storage::DatabasePool& pool);

            // 判断是否属于本地域名
            bool IsLocalDomain(const Address::Domain& domain);

            // SELECT / EXAMINE 使用：统计 EXISTS / UNSEEN
            ImapMailboxCounter LoadImapMailboxCounter(uint64_t mailBoxId, uint64_t folderId);

            // FETCH 使用：sequence number -> 邮件映射
            std::optional<ImapFetchMessage> GetMessageBySequence(uint64_t mailBoxId, uint64_t folderId, uint64_t sequence);

            // UID FETCH 使用：imapUid -> 邮件映射
            std::optional<ImapFetchMessage> GetMessageByUid(uint64_t mailBoxId, uint64_t folderId, uint64_t imapUid);

            // STATUS 指令使用：获取邮件状态
            bool GetMailboxStatus( uint64_t mailBoxId, uint64_t folderId, uint64_t& messages, uint64_t& unseen, uint64_t& recent);

            // FETCH BODY[] 会设置 \Seen；BODY.PEEK[] 不设置
            bool MarkSeen(uint64_t mappingId);

            // 更新 flags
            bool UpdateMessageFlags(uint64_t mappingId, ImapMessageFlag flags);

            // 根据 mappingId 移动
            MoveMailResult MoveMailMapping(uint64_t mappingId, uint64_t targetFolderId);

            // 根据 UID 移动，方便 IMAP UID MOVE 调用
            MoveMailResult MoveMailByUid(uint64_t mailBoxId, uint64_t sourceFolderId, uint64_t imapUid, uint64_t targetFolderId);

            // 发件人与邮件关联：用于站内用户发信。后续会更新发送状态。
            MailMappingCreateResult AddSenderMailMapping(uint64_t mailBoxId, uint64_t mailId, uint64_t folderId, MailMappingStatus status = MailMappingStatus::Queued);

            // 更新发件人的投递状态：QUEUED / SENDING / SENT / FAILED
            bool UpdateSenderMailStatus(uint64_t mailBoxId, uint64_t mailId, MailMappingStatus status);

            // 收件人与邮件关联：本地投递完成时一次性批量写入。
            AddRecipientMappingsResult AddRecipientMailMappings(uint64_t mailId, const std::vector<RecipientMapping>& recipients, uint64_t defaultInboxFolderId);
        };
    }
}
