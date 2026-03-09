// LikeHubServerMain.cpp
#include <Connection.hpp>
#include <LikesProgram/net/Server.hpp>
#include <LikesProgram/net/pollers/EpollPoller.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <iostream>

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
}

int main() {
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
    LikeHub::LikeHubConfig config = LikeHub::LikeHubConfig::Instance(&ApplyConfigDefaultsIfMissing);
    config.LoadConfig(); // 读取配置文件
    config.SaveConfig(); // 重新保存写入，添加未配置的项目

    // 访问原始配置
    LikesProgram::Configuration rawConfig = config.Raw();
    
    // 读取 配置项
    unsigned short port = static_cast<unsigned short>(rawConfig[u"server"][u"port"].AsInt64());
    size_t subLoops = static_cast<size_t>(rawConfig[u"server"][u"eventLoopThreads"].AsInt64());

    // 便利数组
    std::vector<LikesProgram::String> addrs;
    for (const auto& address : rawConfig[u"server"][u"address"]) addrs.push_back(address.AsString());

    // 创建服务器并运行
    LikesProgram::Net::Server server(addrs, port, pollerFactory, connectionFactory, subLoops);
    server.Start();

    // 等待服务器停止（Shutdown 被调用）
    server.WaitShutdown();
    return 0;
}