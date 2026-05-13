// LikeHubServerMain.cpp
#include <Connection.hpp>
#include <DefaultEventHandlers.hpp>
#include <LikesProgram/net/Server.hpp>
#include <LikesProgram/net/transports/TcpTransport.hpp>
#include <LikesProgram/net/pollers/EpollPoller.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <LikesProgram/log/sinks/ConsoleSink.hpp>
#include <LikesProgram/log/sinks/FileSink.hpp>

void ApplyConfigDefaultsIfMissing(LikesProgram::Configuration& config) {
    // server
    LikeHub::LikeHubConfig::EnsureObject(config, u"server");
    LikeHub::LikeHubConfig::EnsureKey(config[u"server"], u"port", 2525);
    unsigned int cpu = std::thread::hardware_concurrency();
    if (cpu == 0) cpu = 4; // fallback（极少数平台返回 0）
    LikeHub::LikeHubConfig::EnsureKey(config[u"server"], u"eventLoopThreads", static_cast<int>(cpu));
    LikesProgram::Configuration addrList; // 数组
    addrList.Push_back(u"*");
    LikeHub::LikeHubConfig::EnsureArray(config[u"server"], u"address", addrList);

    // Authentication
    LikeHub::LikeHubConfig::EnsureObject(config, u"authentication");
    LikeHub::LikeHubConfig::EnsureKey(config[u"authentication"], u"access", "nov_amail_hub_server");
    LikeHub::LikeHubConfig::EnsureKey(config[u"authentication"], u"secret", "nov_amail_hub_server");
    LikesProgram::Configuration whiteList; // 数组
    whiteList.Push_back(u"*");
    LikeHub::LikeHubConfig::EnsureArray(config[u"authentication"], u"whitelist", whiteList);

    // Communication
    LikeHub::LikeHubConfig::EnsureObject(config, u"communication");
    LikeHub::LikeHubConfig::EnsureKey(config[u"communication"], u"heartbeatInterval", 30); // 心跳间隔，单位：秒
    LikeHub::LikeHubConfig::EnsureKey(config[u"communication"], u"heartbeatTimeoutLimit", 3); // 心跳超时次数上限，达到后关闭连接
    LikeHub::LikeHubConfig::EnsureKey(config[u"communication"], u"requestTimeoutMilliseconds", 10 * 1000); // 请求超时时间，单位：毫秒
    LikeHub::LikeHubConfig::EnsureKey(config[u"communication"], u"requestRetryCount", 3); // 普通请求最大重试次数
    LikeHub::LikeHubConfig::EnsureKey(config[u"communication"], u"requestRetryEnabled", true); // 是否允许请求超时自动重试

    // Logger
    LikeHub::LikeHubConfig::EnsureObject(config, u"logger");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"level", u"info");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"path", u"./logs");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"filename", u"HubServer.log");
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"max_file_size", 30);
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"file", false);
    LikeHub::LikeHubConfig::EnsureKey(config[u"logger"], u"console", true);
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

    // 创建轮询器工厂
    LikesProgram::Net::PollerFactory pollerFactory = []()
    -> std::unique_ptr<LikesProgram::Net::Poller> {
        return std::make_unique<LikesProgram::Net::EpollPoller>(nullptr);
    };

    // 创建连接器工厂
    LikesProgram::Net::ConnectionFactory connectionFactory = [](SocketType fd, LikesProgram::Net::EventLoop* ownerLoop)
    -> std::shared_ptr<LikesProgram::Net::Connection> {
        auto transport = std::make_unique<LikesProgram::Net::TcpTransport>(fd);
        return std::make_shared<LikeHub::Connection>(fd, ownerLoop, std::move(transport));
    };

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
    
    // 读取 配置项
    uint16_t port = static_cast<uint16_t>(rawConfig[u"server"][u"port"].AsInt64());
    size_t subLoops = static_cast<size_t>(rawConfig[u"server"][u"eventLoopThreads"].AsInt64());

    // 构建监听地址
    std::vector<LikesProgram::Net::Address> listenAddrs;
    for (const auto& address : rawConfig[u"server"][u"address"]) listenAddrs.push_back(LikesProgram::Net::Address(address.AsString().ToStdString(), port));

    // 创建服务器
    LikesProgram::Net::Server server(listenAddrs, pollerFactory, connectionFactory, subLoops);

    // 配置默认处理事件
    LikeHub::RegisterDefaultEventHandlers();
    // 穷的那个
    server.Start();
    LogInfo(u"NoveMailHub Server started on port {}, with {} event loop threads, startup time: {}", std::to_string(port), std::to_string(subLoops), FormatDuration(timer.Stop()));

    // 等待服务器停止（Shutdown 被调用）
    server.WaitShutdown();
    return 0;
}