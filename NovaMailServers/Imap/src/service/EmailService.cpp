#include <service/EmailService.hpp>
#include <LikeHub/storage/fileSystem/FileSystem.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <LikeHub/mailParser/MailParser.hpp>
#include <LikeHub/Snowflake.hpp>
#include <streambuf>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace NovaMail {
    namespace Imap::Service {
        class BufferStreamBuf : public std::streambuf {
        public:
            BufferStreamBuf(const char* data, size_t size) {
                char* begin = const_cast<char*>(data);
                setg(begin, begin, begin + size);
            }
        protected:
            // 当前读取字符（不移动指针）
            int_type underflow() override {
                if (gptr() < egptr()) {
                    return traits_type::to_int_type(*gptr());
                }
                return traits_type::eof();
            }
        
            // 读取并移动指针
            int_type uflow() override {
                if (gptr() < egptr()) {
                    int_type ch = traits_type::to_int_type(*gptr());
                    gbump(1);
                    return ch;
                }
                return traits_type::eof();
            }
        
            // 批量读取优化
            std::streamsize xsgetn(char* s, std::streamsize count) override {
                std::streamsize avail = egptr() - gptr();
                std::streamsize n = std::min(avail, count);
            
                if (n > 0) {
                    std::memcpy(s, gptr(), n);
                    gbump(static_cast<int>(n));
                }
                return n;
            }
        
            // 支持 seek
            pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override {
                if (!(which & std::ios_base::in)) return pos_type(off_type(-1));
                char* newpos = nullptr;
                            
                if (dir == std::ios_base::beg) {
                    newpos = eback() + off;
                } else if (dir == std::ios_base::cur) {
                    newpos = gptr() + off;
                } else if (dir == std::ios_base::end) {
                    newpos = egptr() + off;
                }
            
                if (newpos < eback() || newpos > egptr())
                    return pos_type(off_type(-1));
            
                setg(eback(), newpos, egptr());
                return pos_type(newpos - eback());
            }
        
            pos_type seekpos(pos_type pos, std::ios_base::openmode which) override {
                return seekoff(pos, std::ios_base::beg, which);
            }
        };

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

        static std::string BuildMailPath(uint64_t mailId) {
            constexpr uint64_t EPOCH = 1609459200000ULL;

            // 从 Snowflake 提取时间
            uint64_t ts = (mailId >> 22) + EPOCH;

            std::time_t sec = ts / 1000;
            std::tm tm{};
            gmtime_r(&sec, &tm);  // UTC，避免多节点问题

            std::ostringstream oss;

            // 日期路径
            oss << std::put_time(&tm, "%Y/%m/%d/");

            // hash 前缀（建议用扰动后的）
            uint64_t h = std::hash<uint64_t>{}(mailId);

            uint8_t b1 = (h >> 0) & 0xFF;
            uint8_t b2 = (h >> 8) & 0xFF;

            oss << std::hex
                << std::setw(2) << std::setfill('0') << (int)b1
                << std::setw(2) << std::setfill('0') << (int)b2;

            // 16位 hex mailId（固定长度）
            oss << std::setw(16) << std::setfill('0') << mailId;

            // 后缀
            oss << ".eml";

            return oss.str();
        }

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

        EmailService::EmailService(LikeHub::Storage::DatabasePool& pool): IService(pool) { }

        bool EmailService::InsertMail(std::string_view storageBucket, uint64_t& mailId, std::string& filePath, const Types::MailFrom& mailFrom, const LikesProgram::Net::Buffer& mailData) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return false;

            LikeHub::MimeMessage parsedMail;
            if (!LikeHub::MailParser::Parse(mailData, parsedMail)) {
                parsedMail.displayName = "Display name parsing failed!";
                parsedMail.subject = "Subject parsing failed!";
                parsedMail.preview = "Content parsing failed!";
            }

            constexpr size_t PREVIEW_MAX = 1024;
            std::string preview;
            preview.assign(parsedMail.preview, 0, PREVIEW_MAX);

            try {
                mailId = LikeHub::Snowflake::NextId();
                filePath = BuildMailPath(mailId);

                std::string senderName;
                if (!mailFrom.DisplayName().empty()) senderName = mailFrom.DisplayName();
                else if (!parsedMail.displayName.empty()) senderName = parsedMail.displayName;
                else if (!parsedMail.from.empty()) senderName = parsedMail.from;
                else senderName = mailFrom.Address().ToString();

                auto stmt = (*conn)->sql(
                    "INSERT INTO Mails(mailId, senderName, senderEmail, `subject`, preview, filePath, storageBucket, `status`) "
                    "VALUES(?, ?, ?, ?, ?, ?, ?, ?);"
                );
                stmt.bind(std::to_string(mailId));
                stmt.bind(senderName);
                stmt.bind(mailFrom.Address().ToString());
                stmt.bind(parsedMail.subject);
                stmt.bind(preview);
                stmt.bind(filePath);
                stmt.bind(std::string(storageBucket));
                stmt.bind("STORING");

                LogInfo(u"Insert mail: mailId={}, senderName={}, mailFrom={}, subject={}, previewContent={}, filePath={}, storageBucket={}",
                    mailId, senderName, mailFrom.Address().ToString(), parsedMail.subject, preview, filePath,  std::string(storageBucket));
                    
                auto result = stmt.execute();
            
                return result.getAffectedItemsCount() == 1;
            } catch (const std::exception& e) {
                LogError(u"InsertMail failed: mailId={}, err={}", mailId, e.what());
                return false;
            }
        }

        bool EmailService::TryMarkProcessing(uint64_t mailId) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return false;
            try {
                auto result = (*conn)->sql(
                    "UPDATE Mails SET status='PROCESSING' "
                    "WHERE mailId=? AND status='STORING';"
                ).bind(mailId).execute();

                return result.getAffectedItemsCount() == 1;
            } catch (const std::exception& e) {
                LogError(u"Try mark processing failed: {}", e.what());
                return false;
            }
        }

        bool EmailService::MarkStored(uint64_t mailId) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return false;
            try {
                auto result = (*conn)->sql(
                    "UPDATE Mails SET status='STORED' "
                    "WHERE mailId=? AND status='PROCESSING';"
                ).bind(mailId).execute();

                return result.getAffectedItemsCount() == 1;
            } catch (const std::exception& e) {
                LogError(u"Try mark processing failed: {}", e.what());
                return false;
            }
        }

        bool EmailService::MarkFailed(uint64_t mailId) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return false;
            try {
                auto result = (*conn)->sql(
                    "UPDATE Mails SET status='FAILED' "
                    "WHERE mailId=? AND status='PROCESSING';"
                ).bind(mailId).execute();

                return result.getAffectedItemsCount() == 1;
            } catch (const std::exception& e) {
                LogError(u"Try mark processing failed: {}", e.what());
                return false;
            }
        }

        bool EmailService::StortMail(std::string_view storageBucket, std::string_view filePath, const LikesProgram::Net::Buffer& mailData) {
            LikeHub::Storage::IFileSystem* fileSystem = LikeHub::Storage::FileSystem::Instance().GetIFileSystem();
            if (!fileSystem) {
                LogError(u"Mail storage failed: filesystem not initialized");
                return false;
            }
        
            const void* data = mailData.Peek();
            const size_t size = mailData.ReadableBytes();
        
            if (size == 0) {
                LogWarn(u"Mail storage skipped: empty data, bucket={} path={}", std::string(storageBucket), std::string(filePath));
                return false;
            }
        
            try {
                std::string errorMessage;
            
                // 构造流
                BufferStreamBuf buf(reinterpret_cast<const char*>(data), size);
                std::istream stream(&buf);
            
                // 避免重复上传
                if (fileSystem->Exists(std::string(storageBucket), std::string(filePath))) {
                    LogDebug(u"Mail storage skipped: already exists, bucket={} path={} size={}", std::string(storageBucket), std::string(filePath), std::to_string(size));
                    return true; // 文件已存在
                }
            
                // 确保从头开始
                stream.clear();
                stream.seekg(0, std::ios::beg);
            
                // 上传
                if (!fileSystem->Upload(std::string(storageBucket), std::string(filePath), stream, errorMessage)) {
                    LogError(u"Mail storage failed: bucket={} path={} size={} error={}", std::string(storageBucket), std::string(filePath), std::to_string(size), errorMessage);
                    return false;
                }
            
                LogDebug(u"Mail stored successfully: bucket={} path={} size={}", std::string(storageBucket), std::string(filePath), std::to_string(size));
                
                return true;
                
            } catch (const std::exception& e) {
                LogError(u"Mail storage exception: bucket={} path={} size={} error={}", std::string(storageBucket), std::string(filePath), std::to_string(size), e.what());
                return false;
                
            } catch (...) {
                LogError(u"Mail storage unknown exception: bucket={} path={} size={}", std::string(storageBucket), std::string(filePath), std::to_string(size));
                return false;
            }
        }

        bool EmailService::GetMailSize(uint64_t mailId, uint64_t& size) {
            size = 0;
            if (mailId == 0) {
                LogError(u"GetMailSize failed: invalid mailId=0");
                return false;
            }
            LikeHub::Storage::IFileSystem* fileSystem = LikeHub::Storage::FileSystem::Instance().GetIFileSystem();
            if (!fileSystem) {
                LogError(u"Get mail size failed: filesystem not initialized");
                return false;
            }
        
            auto conn = m_dbPool.Acquire();
            if (!conn) {
                LogError(u"Get mail size  failed: database connection unavailable, mailId={}", static_cast<int64_t>(mailId));
                return false;
            }
        
            try {
                auto result = (*conn)->sql(
                    "SELECT storageBucket, filePath FROM Mails "
                    "WHERE mailId = ? AND `status` = 'STORED' "
                    "LIMIT 1;"
                ).bind(mailId).execute();
            
                auto row = result.fetchOne();
                if (!row) {
                    LogWarn(
                        u"Get mail size  failed: mail not found or not stored, mailId={}", static_cast<int64_t>(mailId)
                    );
                    return false;
                }
            
                const std::string bucket = row[0].isNull() ? "" : row[0].get<std::string>();
                const std::string filePath = row[1].isNull() ? "" : row[1].get<std::string>();
            
                if (bucket.empty() || filePath.empty()) {
                    LogWarn(
                        u"Get mail size  failed: empty storage path, mailId={} bucket={} filePath={}",
                        static_cast<int64_t>(mailId), bucket, filePath
                    );
                    return false;
                }
            
                std::string errorMsg;
            
                if (!fileSystem->GetObjectSize(bucket, filePath, size, errorMsg)) {
                    LogError(
                        u"Get mail size  failed: mailId={} bucket={} filePath={} error={}",
                        static_cast<int64_t>(mailId), bucket, filePath, errorMsg
                    );
                    return false;
                }
            
                return true;
            } catch (const std::exception& e) {
                LogError(u"Get mail size  failed: mailId={} error={}", static_cast<int64_t>(mailId), e.what());
                return false;
            }
        }

        bool EmailService::GetMailData(uint64_t mailId, LikesProgram::Net::Buffer& mailData) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return false;
            try {
                auto result = (*conn)->sql(
                    "SELECT filePath, storageBucket FROM Mails WHERE mailId = ? AND status='STORED' LIMIT 1;"
                ).bind(mailId).execute();

                mysqlx::Row row = result.fetchOne();
                if (!row) return false;

                std::string filePath = row[0].get<std::string>();
                std::string storageBucket = row[1].get<std::string>();

                return GetMailFileData(storageBucket, filePath, mailData);
            } catch (const std::exception& e) {
                LogError(u"InsertMail failed: {}", e.what());
                return false;
            }
        }

        bool EmailService::AddLabel(uint64_t mailBoxMailId, uint64_t labelId) {
            auto conn = m_dbPool.Acquire();
            if (!conn) return false;
            try {
                auto result = (*conn)->sql(
                    "INSERT INTO MailLabelsMapping(mailBoxMailId, labelId) VALUES(?, ?);"
                ).bind(mailBoxMailId).bind(labelId).execute();
            
                return result.getAffectedItemsCount() == 1;
            } catch (const std::exception& e) {
                LogError(u"InsertMail failed: mailBoxMailId={}, err={}", mailBoxMailId, e.what());
                return false;
            }
        }
    }
}
