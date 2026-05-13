#include <Connection.hpp>
#include <LikesProgram/net/Server.hpp>
#include <LikeHub/LikeHubClient.hpp>
#include <LikesProgram/net/transports/TcpTransport.hpp>
#include <LikesProgram/net/pollers/EpollPoller.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <LikesProgram/log/sinks/ConsoleSink.hpp>
#include <LikesProgram/log/sinks/FileSink.hpp>
#include <LikeHub/storage/database/DatabasePool.hpp>
#include <LikesProgram/time/Timer.hpp>
#include <LikeHub/storage/fileSystem/FileSystem.hpp>
#include <LikeHub/storage/fileSystem/MinioClient.hpp>
#include <LikeHub/SecurityContextManager.hpp>
#include <cstdlib>
#include <memory>

// 配置缺省值
void ApplyConfigDefaultsIfMissing(LikesProgram::Configuration& config) {
    // Imap
    LikeHub::LikeHubConfig::EnsureObject(config, u"imap");
    LikeHub::LikeHubConfig::EnsureKey(config[u"imap"], u"greeting", u"NovaMail IMAP Server");
    LikeHub::LikeHubConfig::EnsureKey(config[u"imap"], u"hostname", u"imap.lby.email");
    LikeHub::LikeHubConfig::EnsureKey(config[u"imap"], u"start_tls", false);

    // Server
    LikeHub::LikeHubConfig::EnsureObject(config, u"server");
    LikeHub::LikeHubConfig::EnsureKey(config[u"server"], u"port", 143);

    unsigned int cpu = std::thread::hardware_concurrency();
    if (cpu == 0) cpu = 4; // fallback（极少数平台返回 0）
    LikeHub::LikeHubConfig::EnsureKey(config[u"server"], u"eventLoopThreads", static_cast<int>(cpu));
    
    LikesProgram::Configuration addrList; // 数组
    addrList.Push_back(u"*");
    LikeHub::LikeHubConfig::EnsureArray(config[u"server"], u"address", addrList);
    
    // NovaMailHub
    LikeHub::LikeHubConfig::EnsureObject(config, u"hubServer");
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"serviceName", u"NovaMailImtp");
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"ip", u"127.0.0.1");
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"port", 2525);
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"access", u"nov_amail_hub_server");
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"secret", u"nov_amail_hub_server");
    LikeHub::LikeHubConfig::EnsureKey(config[u"hubServer"], u"tls", false);

    // Logger
    LikeHub::LikeHubConfig::EnsureObject(config, u"logger");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"level", u"info");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"path", u"./logs");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"filename", u"ImtpServer.log");
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

    // 读取 服务器 配置项
    unsigned short port = static_cast<unsigned short>(rawConfig[u"server"][u"port"].AsInt64()); // 端口号
    size_t subLoops = static_cast<size_t>(rawConfig[u"server"][u"eventLoopThreads"].AsInt64()); // 事件循环线程数
    // 构建监听地址
    std::vector<LikesProgram::Net::Address> listenAddrs;
    for (const auto& address : rawConfig[u"server"][u"address"]) listenAddrs.push_back(LikesProgram::Net::Address(address.AsString().ToStdString(), port));

    // 创建轮询器工厂
    LikesProgram::Net::PollerFactory pollerFactory = []()
    -> std::unique_ptr<LikesProgram::Net::Poller> {
        return std::make_unique<LikesProgram::Net::EpollPoller>(nullptr);
    };

    // 创建连接器工厂
    LikesProgram::Net::ConnectionFactory connectionFactory = [](SocketType fd, LikesProgram::Net::EventLoop* ownerLoop)
    -> std::shared_ptr<LikesProgram::Net::Connection> {
        auto transport = std::make_unique<LikesProgram::Net::TcpTransport>(fd);
        return std::make_shared<NovaMail::Imap::Connection>(fd, ownerLoop, std::move(transport));
    };

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

    // 初始化证书管理
    LikeHub::SecurityContextManager::Initialize(LikeHub::Storage::DatabasePool::Instance());

    // 创建服务器
    auto server = std::make_shared<LikesProgram::Net::Server>(listenAddrs, pollerFactory, connectionFactory, subLoops);

    // 初始化 Hub 客户端
    LikesProgram::String hubServerIp = rawConfig[u"hubServer"][u"ip"].AsString();
    unsigned short hubServerPort = static_cast<unsigned short>(rawConfig[u"hubServer"][u"port"].AsInt64()); // 端口号
    LikesProgram::String hubServerServiceName = rawConfig[u"hubServer"][u"serviceName"].AsString();
    LikesProgram::String hubServerAccess = rawConfig[u"hubServer"][u"access"].AsString();
    LikesProgram::String hubServerSecret = rawConfig[u"hubServer"][u"secret"].AsString();
    LikeHub::LikeHubClient::Initialize(LikesProgram::Net::Address(
        hubServerIp.ToStdString(), hubServerPort), hubServerServiceName, hubServerAccess, hubServerSecret, 
    [server](){
        // 客户端连接成功，并完成登录
        LogInfo(u"NovaMailImap connected to LikeHub and logged in successfully");
        // 启动服务器
        server->Start();
    }, [](std::string message){
        // 客户端启动失败或登录失败
        LogInfo(u"NovaMailImap failed to connect/login LikeHub: {}", message);

        // 退出程序
        std::exit(EXIT_FAILURE);
    });

    // 等待服务器启动
    while (server->GetStatus() != LikesProgram::Net::Server::Status::Running);
    LogInfo(u"IMAP Server started on port {}, with {} event loop threads, startup time: {}", std::to_string(port), std::to_string(subLoops), FormatDuration(timer.Stop()));

    // 等待服务器停止（Shutdown 被调用）
    server->WaitShutdown();
    LogInfo(u"IMAP Server stopped.");
    return 0;
}
