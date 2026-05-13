#include <EmailService.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <LikeHub/storage/fileSystem/FileSystem.hpp>
#include <optional>
#include <LikesProgram/Configuration.hpp>
#include <LikeHub/RpcMessage.hpp>
#include <LikeHub/LikeHubClient.hpp>

namespace NovaMail::Classification {
    static std::optional<uint64_t> GetTargetFolderIdByLabelId(uint64_t labelId) {
        switch (labelId) {
            case 1: return 2; // 正常 -> INBOX
            case 2: return 5; // 广告 -> SPAM
            case 3: return 4; // 垃圾 -> TRASH
            case 4: return 4; // 未知来源 -> TRASH
            default:
                return std::nullopt;
        }
    }

    class BufferWriteStreamBuf : public std::streambuf {
    public:
        explicit BufferWriteStreamBuf(LikesProgram::Net::Buffer& buf) : buffer(buf) {}
    protected:
        std::streamsize xsputn(const char* s, std::streamsize n) override {
            if (n <= 0) return 0;
            buffer.Append(s, static_cast<size_t>(n));
            return n;
        }

        int_type overflow(int_type ch) override {
            if (traits_type::eq_int_type(ch, traits_type::eof())) {
                return traits_type::not_eof(ch);
            }

            char c = traits_type::to_char_type(ch);
            buffer.Append(&c, 1);
            return ch;
        }

        int sync() override {
            return 0;
        }
    private:
        LikesProgram::Net::Buffer& buffer;
    };

    EmailService::EmailService(LikeHub::Storage::DatabasePool& pool): IService(pool), m_mailBoxService(pool) { }

    static bool GetMailFileData(std::string_view storageBucket, std::string_view filePath, LikesProgram::Net::Buffer& mailData) {
        LikeHub::Storage::IFileSystem* fileSystem = LikeHub::Storage::FileSystem::Instance().GetIFileSystem();
        if (!fileSystem) {
            LogError(u"Mail storage failed: filesystem not initialized");
            return false;
        }
        try {
            std::string errorMessage;
            // 清空旧数据（很关键）
            mailData.RetrieveAll();
            BufferWriteStreamBuf buf(mailData);
            std::ostream stream(&buf);
            // 文件不存在
            if (!fileSystem->Exists(std::string(storageBucket), std::string(filePath))) return false;
            if (!fileSystem->Download(std::string(storageBucket), std::string(filePath), stream, errorMessage)) {
                LogError(u"Mail load failed: bucket={} path={} error={}", std::string(storageBucket), std::string(filePath), errorMessage);
                return false;
            }
            LogDebug(u"Mail loaded successfully: bucket={} path={} size={}", std::string(storageBucket), std::string(filePath), std::to_string(mailData.ReadableBytes()));
        
            return true;
        } catch (const std::exception& e) {
            LogError(u"Mail load exception: bucket={} path={} error={}", std::string(storageBucket), std::string(filePath), e.what());
        
            return false;
        } catch (...) {
            LogError(u"Mail load unknown exception: bucket={} path={}", std::string(storageBucket), std::string(filePath));
        
            return false;
        }
    }

    bool EmailService::GetMailData(uint64_t mailId, LikesProgram::Net::Buffer& mailData) {
        auto conn = m_dbPool.Acquire();
        if (!conn) return false;
        try {
            auto result = (*conn)->sql(
                "SELECT filePath, storageBucket FROM Mails WHERE mailId = ? AND (status='STORED' OR status='SENT') LIMIT 1;"
            ).bind(mailId).execute();

            mysqlx::Row row = result.fetchOne();
            if (!row) return false;

            std::string filePath = row[0].get<std::string>();
            std::string storageBucket = row[1].get<std::string>();

            return GetMailFileData(storageBucket, filePath, mailData);
        } catch (const std::exception& e) {
            LogError(u"GetMailData failed: {}", e.what());
            return false;
        }
    }

    struct NeedMoveMapping {
        uint64_t mappingId;
        uint64_t mailBoxId;
        uint64_t folderId;
        bool isRead;
    };
    bool EmailService::AddLabel(uint64_t mailId, uint64_t labelId) {
        auto conn = m_dbPool.Acquire();
        if (!conn) return false;

        std::vector<NeedMoveMapping> mappings;
        auto targetFolderId = GetTargetFolderIdByLabelId(labelId);

        try {
            (*conn)->startTransaction();

            /*
             * 查询该邮件在所有收件邮箱中的映射。
             * 这里不只查需要移动的记录，因为没有移动也需要通知前端。
             */
            auto mappingResult = (*conn)->sql(
                "SELECT mappingId, mailBoxId, folderId, isRead "
                "FROM MailBoxMailMapping "
                "WHERE mailId = ? AND mappingType = 'TO';"
            ).bind(mailId).execute();

            for (;;) {
                auto row = mappingResult.fetchOne();
                if (!row) break;

                if (row[0].isNull() || row[1].isNull() || row[2].isNull() || row[3].isNull()) {
                    continue;
                }

                mappings.push_back(NeedMoveMapping{
                    row[0].get<uint64_t>(),
                    row[1].get<uint64_t>(),
                    row[2].get<uint64_t>(),
                    row[3].get<int>() != 0
                });
            }

            if (mappings.empty()) {
                (*conn)->rollback();
                return false;
            }

            /*
             * 标签绑定做幂等处理。
             */
            (*conn)->sql(
                "INSERT IGNORE INTO MailLabelsMapping(mailBoxMailId, labelId) "
                "SELECT mappingId, ? "
                "FROM MailBoxMailMapping "
                "WHERE mailId = ? AND mappingType = 'TO';"
            ).bind(labelId).bind(mailId).execute();

            (*conn)->commit();
        } catch (const std::exception& e) {
            try { (*conn)->rollback(); } catch (...) { }

            LogError(
                u"AddLabel failed: mailId={}, labelId={}, err={}",
                static_cast<int64_t>(mailId),
                static_cast<int64_t>(labelId),
                e.what()
            );

            return false;
        }

        LikeHub::LikeHubClient& likeHub = LikeHub::LikeHubClient::Instance();

        for (const auto& item : mappings) {
            uint64_t notifyFolderId = item.folderId;

            // 只有满足条件时才移动
            if (targetFolderId.has_value() && !item.isRead && item.folderId != *targetFolderId) {
                auto move = m_mailBoxService.MoveMailMapping(item.mappingId, *targetFolderId);

                if (!move.success) {
                    LogWarn(
                        u"Classification move mapping failed: mailId={} labelId={} mappingId={} targetFolderId={}",
                        static_cast<int64_t>(mailId),
                        static_cast<int64_t>(labelId),
                        static_cast<int64_t>(item.mappingId),
                        static_cast<int64_t>(*targetFolderId)
                    );

                    // 移动失败时，通知仍然发送，但 folderId 使用原文件夹
                    notifyFolderId = item.folderId;
                } else {
                    // 移动成功后，通知目标文件夹
                    notifyFolderId = *targetFolderId;
                }
            }

            // 无论是否移动，都发送通知
            LikesProgram::Configuration newEmailDeliveryMetadata;
            newEmailDeliveryMetadata[u"action"] = u"NewEmailDelivery";
            newEmailDeliveryMetadata[u"mappingId"] = LikesProgram::String(std::to_string(item.mappingId));
            newEmailDeliveryMetadata[u"mailBoxId"] = LikesProgram::String(std::to_string(item.mailBoxId));
            newEmailDeliveryMetadata[u"mailId"] = LikesProgram::String(std::to_string(mailId));
            newEmailDeliveryMetadata[u"folderId"] = LikesProgram::String(std::to_string(notifyFolderId));
            likeHub.SendMessage(LikeHub::RpcMessage::Encode(LikeHub::RpcMessage::Type::Request, 0, LikeHub::RpcMessage::Code::None, newEmailDeliveryMetadata, 0), 0);
        }

        return true;
    }
}