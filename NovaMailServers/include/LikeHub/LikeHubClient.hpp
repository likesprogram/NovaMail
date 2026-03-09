#pragma once
#include <string>

namespace LikeHub {
    class LikeHubClient {
    public:
        LikeHubClient(const std::string& serverAddress, int port);
        ~LikeHubClient();

        // 连接到服务器
        bool Connect(
            const std::string& serviceName,
            const std::string& instanceName,
            const std::string& ipAddress,
            uint16_t port,
            const std::string& password = ""
        );

        // 等待 断开连接 (卡住当前线程，直到连接断开)
        void WaitForDisconnect();

        // 主动断开连接
        void Shutdown();

    private:
    };
}