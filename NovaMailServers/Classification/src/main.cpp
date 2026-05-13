#include <LikeHub/mailParser/MailParser.hpp>
#include <LikesProgram/net/Buffer.hpp>
#include <BertMailClassifier.hpp>
#include <LikeHub/LikeHubClient.hpp>
#include <LikeHub/EventListener.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <LikesProgram/log/sinks/ConsoleSink.hpp>
#include <LikesProgram/log/sinks/FileSink.hpp>
#include <LikeHub/storage/database/DatabasePool.hpp>
#include <LikesProgram/time/Timer.hpp>
#include <LikeHub/storage/fileSystem/FileSystem.hpp>
#include <LikeHub/storage/fileSystem/MinioClient.hpp>
#include <EmailService.hpp>

// 配置缺省值
void ApplyConfigDefaultsIfMissing(LikesProgram::Configuration& config) {
    // ModelFile
    LikeHub::LikeHubConfig::EnsureObject(config, u"models");
    LikeHub::LikeHubConfig::EnsureKey(config[u"models"], u"modelPath", u"./models/NovaMailClassification_1_0_2/bert.onnx");
    LikeHub::LikeHubConfig::EnsureKey(config[u"models"], u"vocabPath", u"./models/NovaMailClassification_1_0_2/vocab.txt");
    LikeHub::LikeHubConfig::EnsureKey(config[u"models"], u"labelMapPath", u"./models/NovaMailClassification_1_0_2/label_map.json");
    LikeHub::LikeHubConfig::EnsureKey(config[u"models"], u"maxLength", 256);

    // NovaMailHub
    LikeHub::LikeHubConfig::EnsureObject(config, u"hubServer");
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"serviceName", u"NovaMailClassification");
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"ip", u"127.0.0.1");
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"port", 2525);
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"access", u"nov_amail_hub_server");
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"secret", u"nov_amail_hub_server");
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"tls", false);

    // Logger
    LikeHub::LikeHubConfig::EnsureObject(config, u"logger");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"level", u"info");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"path", u"./logs");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"filename", u"NovaMailClassification.log");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"max_file_size", 30);
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"file", false);
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"console", true);

    // DataBase
    LikeHub::LikeHubConfig::EnsureObject(config, u"database");
    LikeHub::LikeHubConfig::EnsureKey(config[u"database"], u"uri", u"mysqlx://root:password@127.0.0.1:33060/NovaMailDatabase");
    LikeHub::LikeHubConfig::EnsureKey(config[u"database"], u"pool_size", 16);

    // FileStorage
    LikeHub::LikeHubConfig::EnsureObject(config, u"storage");
    LikeHub::LikeHubConfig::EnsureKey(config[u"storage"], u"endpoint", u"192.168.235.129:9000");
    LikeHub::LikeHubConfig::EnsureKey(config[u"storage"], u"access", u"likesprogram");
    LikeHub::LikeHubConfig::EnsureKey(config[u"storage"], u"secret", u"likesprogram");
    LikeHub::LikeHubConfig::EnsureKey(config[u"storage"], u"https", false);
}

// 格式化持续时间为字符串
LikesProgram::String FormatDuration(LikesProgram::Time::Duration ns) {
    double value = ns.count();

    if (value < 1000.0)
        return LikesProgram::String::Format(u"{:.2f} ns", value);

    value /= 1000.0; // us
    if (value < 1000.0)
        return LikesProgram::String::Format(u"{:.2f} us", value);

    value /= 1000.0; // ms
    if (value < 1000.0)
        return LikesProgram::String::Format(u"{:.2f} ms", value);

    value /= 1000.0; // s
    if (value < 60.0)
        return LikesProgram::String::Format(u"{:.2f} s", value);

    value /= 60.0; // min
    return LikesProgram::String::Format(u"{:.2f} min", value);
}

static uint64_t ParseMailId(const LikeHub::RpcMessage& request) {
    uint64_t mailId = 0;
    try {
        const auto& metadata = request.MetaData();
        if (!metadata[u"mailId"].IsNull()) mailId = static_cast<uint64_t>(metadata[u"mailId"].AsInt64());
    } catch (const std::exception& e) {
        LogWarn(
            u"Hub service request mailId parse failed: error={} requestId={}",
            e.what(), request.Header().requestId
        );
    } catch (...) {
        LogWarn(
            u"Hub service request mailId parse failed: unknown error requestId={}", request.Header().requestId
        );
    }
    return mailId;
}

static LikesProgram::String ParseAction(const LikeHub::RpcMessage& request) {
    LikesProgram::String action = u"Unknown";
    try {
        const auto& metadata = request.MetaData();
        if (!metadata[u"action"].IsNull()) action = metadata[u"action"].AsString();
    } catch (const std::exception& e) {
        LogWarn(
            u"Hub service request mailId parse failed: error={} requestId={}",
            e.what(), request.Header().requestId
        );
    } catch (...) {
        LogWarn(
            u"Hub service request mailId parse failed: unknown error requestId={}", request.Header().requestId
        );
    }
    return action;
}

static uint64_t StringToLabelId(const std::string& label) {
    if (label == "normal") return 1;
    if (label == "advertisement") return 2;
    if (label == "spam") return 3;
    return 0;
}

int main() {
    // 创建计时器，记录启动耗时
    LikesProgram::Time::Timer timer(true); // 自启动
    
    // 读取配置
    LikeHub::LikeHubConfig& config = LikeHub::LikeHubConfig::Instance(&ApplyConfigDefaultsIfMissing);
    config.LoadConfig(); // 读取配置文件
    config.SaveConfig(); // 重新保存写入，添加未配置的项目

    // 访问原始配置
    LikesProgram::Configuration rawConfig = config.Raw();

    // 设置日志级别
    LikesProgram::String levelString = rawConfig[u"logger"][u"level"].AsString(); // 获取 配置 字符串
    LikesProgram::Log::Level level = LikesProgram::Log::StringToLevel(levelString);

    // 初始化日志系统
    auto& logger = LikesProgram::Log::Logger::Instance(true);
    logger.SetLevel(level);

    // 控制台日志
    bool consoleLogging = rawConfig[u"logger"][u"console"].AsBool();
    if (consoleLogging) logger.AddSink(LikesProgram::Log::ConsoleSink::CreateSink());

    // 文件日志
    bool fileLogging = rawConfig[u"logger"][u"file"].AsBool();
    LikesProgram::String logPath = rawConfig[u"logger"][u"path"].AsString();
    LikesProgram::String logFilename = rawConfig[u"logger"][u"filename"].AsString();
    size_t maxFileSize = static_cast<size_t>(rawConfig[u"logger"][u"max_file_size"].AsInt64());
    LogDebug(
        u"[console] Logger initialized with level {}, console={}, file={}, path={}, filename={}, max_file_size={}MB",
        LikesProgram::Log::LevelToString(level),
        consoleLogging ? u"on" : u"off",
        fileLogging ? u"on" : u"off",
        logPath, logFilename, maxFileSize
    );
    if (fileLogging) logger.AddSink(LikesProgram::Log::FileSink::CreateSink(logPath, logFilename, maxFileSize));
    
    // 初始化数据库连接池
    std::string databaseUri = rawConfig[u"database"][u"uri"].AsString().ToStdString();
    size_t poolSize = static_cast<size_t>(rawConfig[u"database"][u"pool_size"].AsInt64());
    LikeHub::Storage::DatabasePool::Initialize(databaseUri, poolSize);

    // 初始化文件系统
    LikesProgram::String endpoint = rawConfig[u"storage"][u"endpoint"].AsString();
    LikesProgram::String access = rawConfig[u"storage"][u"access"].AsString();
    LikesProgram::String secret = rawConfig[u"storage"][u"secret"].AsString();
    bool https = rawConfig[u"storage"][u"https"].AsBool();
    std::shared_ptr<LikeHub::Storage::IFileSystem> iFileSystem = std::make_shared<LikeHub::Storage::MinioClient>(
        endpoint.ToStdString(), access.ToStdString(), secret.ToStdString(), https
    );
    LikeHub::Storage::FileSystem::Instance().Initialize(iFileSystem);

    // 加载模型
    LikesProgram::String modelPath = rawConfig[u"models"][u"modelPath"].AsString();
    LikesProgram::String vocabPath = rawConfig[u"models"][u"vocabPath"].AsString();
    LikesProgram::String labelMapPath = rawConfig[u"models"][u"labelMapPath"].AsString();
    size_t maxLength = static_cast<size_t>(rawConfig[u"models"][u"maxLength"].AsInt64());
    NovaMail::Classification::BertMailClassifier classifier(modelPath.ToStdString(), vocabPath.ToStdString(), labelMapPath.ToStdString(), maxLength);
    LogInfo(
        u"Loadin model successfully: modelPath:{} vocabPath:{} labelMapPath:{} maxLength:{}",
        modelPath, vocabPath, labelMapPath, static_cast<int64_t>(maxLength)
    );

    // 创建 邮件服务实例
    NovaMail::Classification::EmailService emailService(LikeHub::Storage::DatabasePool::Instance());

    // 分类事件处理逻辑，单独提出来，避免 RegisterEvent 内部过度嵌套
    auto classificationHandler = [&classifier, &emailService](const LikeHub::EventContext& ctx) -> bool {
        LikeHub::LikeHubClient& likeHub = LikeHub::LikeHubClient::Instance();
        LikesProgram::Configuration metadata;
        metadata[u"action"] = ParseAction(ctx.message);
        const uint64_t mailId = ParseMailId(ctx.message);
        if (mailId <= 0) {
            LogError(u"Classification failed: invalid mailId={}", std::to_string(mailId));
            // 先给服务端回复，避免超时
            likeHub.SendMessage(LikeHub::RpcMessage::Encode(LikeHub::RpcMessage::Type::Response, ctx.message.Header().requestId, LikeHub::RpcMessage::Code::NotFound, metadata, 0), 0);
            return false;
        }

        LikesProgram::Net::Buffer mailData;
        if (!emailService.GetMailData(mailId, mailData)) {
            LogError(u"Classification failed: load mail data failed, mailId={}", std::to_string(mailId));
            // 先给服务端回复，避免超时
            likeHub.SendMessage(LikeHub::RpcMessage::Encode(LikeHub::RpcMessage::Type::Response, ctx.message.Header().requestId, LikeHub::RpcMessage::Code::NotFound, metadata, 0), 0);
            return false;
        }

        // 先给服务端回复，避免超时
        likeHub.SendMessage(LikeHub::RpcMessage::Encode(LikeHub::RpcMessage::Type::Response, ctx.message.Header().requestId, LikeHub::RpcMessage::Code::OK, metadata, 0), 0);

        LikeHub::MimeMessage parsedMail;
        if (!LikeHub::MailParser::Parse(mailData, parsedMail)) {
            LogError(u"Classification warning: parse mail failed, mailId={}", std::to_string(mailId));

            parsedMail.displayName = "Display name parsing failed!";
            parsedMail.subject = "Subject parsing failed!";
            parsedMail.preview = "Content parsing failed!";
        }

        NovaMail::Classification::BertClassifyResult result = classifier.Predict(parsedMail.preview);
        uint64_t labelId = StringToLabelId(result.label);
        if (labelId == 0) {
            LogError(
                u"Classification failed: label mapping failed, mailId={} predictedLabel={} previewSize={}",
                std::to_string(mailId), result.label, std::to_string(parsedMail.preview.size())
            );
            return false;
        }

        if (!emailService.AddLabel(mailId, labelId)) {
            LogError(
                u"Classification failed: add label to mail failed, mailId={} labelId={} predictedLabel={} previewSize={}",
                std::to_string(mailId), labelId, result.label, std::to_string(parsedMail.preview.size())
            );
            return false;
        }

        LogInfo(
            u"Classification completed: mailId={} labelId={} predictedLabel={} previewSize={}",
            std::to_string(mailId), labelId, result.label, std::to_string(parsedMail.preview.size())
        );

        return true;
    };

    // 登录成功回调
    auto onHubLoginSuccess = [&classificationHandler]() {
        LogInfo(u"NovaMailClassification connected to LikeHub and logged in successfully");
        LikeHub::LikeHubClient& hubServer = LikeHub::LikeHubClient::Instance();
        hubServer.RegisterEvent(u"Classification", classificationHandler, LikeHub::ListenType::Event);
    };

    // 登录失败回调
    auto onHubLoginFailed = [](std::string message) {
        LogError(u"NovaMailClassification failed to connect/login LikeHub: {}", message);
        std::exit(EXIT_FAILURE);
    };

    // 初始化 Hub 客户端
    LikesProgram::String hubServerIp = rawConfig[u"hubServer"][u"ip"].AsString();
    unsigned short hubServerPort = static_cast<unsigned short>(rawConfig[u"hubServer"][u"port"].AsInt64()); // 端口号
    LikesProgram::String hubServerServiceName = rawConfig[u"hubServer"][u"serviceName"].AsString();
    LikesProgram::String hubServerAccess = rawConfig[u"hubServer"][u"access"].AsString();
    LikesProgram::String hubServerSecret = rawConfig[u"hubServer"][u"secret"].AsString();
    LikeHub::LikeHubClient::Initialize(LikesProgram::Net::Address(
        hubServerIp.ToStdString(), hubServerPort), hubServerServiceName,
        hubServerAccess, hubServerSecret, 
        onHubLoginSuccess, onHubLoginFailed
    );
    LikesProgram::Net::Client& client = LikeHub::LikeHubClient::Instance().GetClient();
    client.WaitShutdown(); // 等待客户端停止
    return 0;
}