#include <Connection.hpp>
#include <LikesProgram/net/Server.hpp>
#include <LikesProgram/net/pollers/EpollPoller.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <LikesProgram/log/sinks/ConsoleSink.hpp>
#include <LikesProgram/log/sinks/FileSink.hpp>
#include <LikeHub/database/DataBasePool.hpp>
#include <LikesProgram/time/Timer.hpp>
#include <iostream>

// 配置缺省值
void ApplyConfigDefaultsIfMissing(LikesProgram::Configuration& config) {
    // smtp
    LikeHub::LikeHubConfig::EnsureKey(config, u"greeting", u"NovaMail SMTP Server");
    LikeHub::LikeHubConfig::EnsureKey(config, u"hostname", u"smtp.lby.email");

    // server
    LikeHub::LikeHubConfig::EnsureObject(config, u"server");
    LikeHub::LikeHubConfig::EnsureKey(config[u"server"], u"port", 25);

    unsigned int cpu = std::thread::hardware_concurrency();
    if (cpu == 0) cpu = 4; // fallback（极少数平台返回 0）
    LikeHub::LikeHubConfig::EnsureKey(config[u"server"], u"eventLoopThreads", static_cast<int>(cpu));
    
    LikesProgram::Configuration addrList; // 数组
    addrList.Push_back(u"*");
    LikeHub::LikeHubConfig::EnsureArray(config[u"server"], u"address", addrList);

    // transaction
    LikeHub::LikeHubConfig::EnsureObject(config, u"transaction");
    LikeHub::LikeHubConfig::EnsureKey(config[u"transaction"], u"maxRcptCount", 1000);
    LikeHub::LikeHubConfig::EnsureKey(config[u"transaction"], u"maxDataBytes", 10485760);
    LikeHub::LikeHubConfig::EnsureKey(config[u"transaction"], u"dedupRcpt", true);

    // Logger
    LikeHub::LikeHubConfig::EnsureObject(config, u"logger");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"level", u"info");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"path", u"./logs");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"filename", u"SmtpServer.log");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"max_file_size", 30);
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"file", true);
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"console", true);

    // DataBase
    LikeHub::LikeHubConfig::EnsureObject(config, u"database");
    LikeHub::LikeHubConfig::EnsureKey(config[u"database"], u"uri", u"mysqlx://root:password@127.0.0.1:33060");
    LikeHub::LikeHubConfig::EnsureKey(config[u"database"], u"pool_size", 16);
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
    LikeHub::LikeHubConfig config = LikeHub::LikeHubConfig::Instance(&ApplyConfigDefaultsIfMissing);
    config.LoadConfig(); // 读取配置文件
    config.SaveConfig(); // 重新保存写入，添加未配置的项目

    // 访问原始配置
    LikesProgram::Configuration rawConfig = config.Raw();

    // 初始化日志系统
    auto& logger = LikesProgram::Log::Logger::Instance(true);

    // 设置日志级别
    LikesProgram::String levelString = rawConfig[u"logger"][u"level"].AsString(); // 获取 配置 字符串
    LikesProgram::Log::Level level = LikesProgram::Log::StringToLevel(levelString);
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
    // 便利地址数组
    std::vector<LikesProgram::String> addrs;
    for (const auto& address : rawConfig[u"server"][u"address"]) addrs.push_back(address.AsString()); // 监听地址列表

    // 创建轮询器工厂
    LikesProgram::Net::PollerFactory pollerFactory = []()
    -> std::unique_ptr<LikesProgram::Net::Poller> {
        return std::make_unique<LikesProgram::Net::EpollPoller>(nullptr);
    };

    // 创建连接器工厂
    LikesProgram::Net::ConnectionFactory connectionFactory = [](SocketType fd, LikesProgram::Net::EventLoop* ownerLoop)
    -> std::shared_ptr<LikesProgram::Net::Connection> {
        auto transport = std::make_unique<LikesProgram::Net::TcpTransport>(fd);
        return std::make_shared<NovaMail::Smtp::Connection>(fd, ownerLoop, std::move(transport));
    };

    // 初始化数据库连接池
    std::string databaseUri = rawConfig[u"database"][u"uri"].AsString().ToStdString();
    size_t poolSize = static_cast<size_t>(rawConfig[u"database"][u"pool_size"].AsInt64());
    LikeHub::DataBasePool::Initialize(databaseUri, poolSize);

    // 创建服务器
    LikesProgram::Net::Server server(addrs, port, pollerFactory, connectionFactory, subLoops);

    /* TODO
    * 1、创建 服务发现客户端
    * 2、连接到 服务发现
    * 3、注册事件 【邮件送达】
    */

    // 启动服务器
    server.Start();
    LogInfo(u"SMTP Server started on port {}, with {} event loop threads, startup time: {}", std::to_string(port), std::to_string(subLoops), FormatDuration(timer.Stop()));

    // 等待服务器停止（Shutdown 被调用）
    server.WaitShutdown();
    LogInfo(u"SMTP Server stopped.");
    return 0;
}
