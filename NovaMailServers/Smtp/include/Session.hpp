#pragma once
#include <types/Types.hpp>
#include <command/Command.hpp>
#include <Transaction.hpp>
#include <LikesProgram/Configuration.hpp>
#include <LikesProgram/net/Address.hpp>
#include <command/CommandHandler.hpp>
#include <service/UserService.hpp>
#include <optional>

namespace NovaMail {
    namespace Smtp {
        class Connection;
        class Session : public std::enable_shared_from_this<Session> {
        public:
            Session(Connection* connection, LikesProgram::Net::Address remoteAddr, LikesProgram::Net::Address localAddr);
            ~Session() = default;

            // 初始化处理类
            void InitHandlers();

            void OnConnect(); // 发送 220
            void OnBytes(LikesProgram::Net::Buffer& in); // 处理输入
            bool ShouldClose(bool value = false, bool set = false) noexcept;           // 应用关闭

            void Reply(Types::ReplyCode code, const std::string& message, const std::string& hyphen = " ");

            // 切换状态
            void SetState(Types::State state);
            // 获取当前状态
            Types::State GetState();

            // 开启 TLS 对话
            void StartTls();
            // 当前是否已经是 SSL/TLS 模式
            bool IsTLS();

            // 获取当前 Transaction
            Transaction& GetTransaction();

            // 获取当前会话支持的能力集合
            Types::Capabilities GetCapabilities();

            // 是否开启中继, 强制认证
            bool RelayPolicy() const noexcept;

            // 保存登录的用户
            void SetUser(const Entity::User& user);
            // 已认证
            bool Authenticated() const noexcept;
            const Entity::User* GetUser() const noexcept;

            // 获取 Smtp 服务名称
            std::string SmtpHostname() const noexcept;

            // 获取对端地址
            const LikesProgram::Net::Address& GetRemoteAddress() const noexcept;
            // 获取本段地址
            const LikesProgram::Net::Address& GetLocalAddress() const noexcept;
        private:
            // 尝试解析指令
            std::optional<Command> TryParseCommand(LikesProgram::Net::Buffer& in);

            // 指令派遣处理
            void Dispatch(const Command& command);
        private:
            Connection* m_connection = nullptr;             // 服务控制器
            std::array<std::unique_ptr<CommandHandler>, Types::CommandVerbCount> m_handlers; // 指令处理器
            Types::State m_state = Types::State::Connected; // 当前 SMTP 状态机状态
            LikesProgram::Configuration m_config;           // 配置
            std::string m_smtpHostname = "NovaMail";        // SMTP 服务名称
            Transaction      m_tx;                          // 当前邮件事务容器
            Types::Capabilities m_caps;                     // 当前会话支持的能力集合
            bool             m_isTLS = false;               // 当前是否启用 SLL/TLS
            bool             m_relayPolicy = false;         // 是否强制登录认证
            std::optional<Entity::User> m_authUser;         // 认证的用户
            std::atomic_bool m_shouldClose{false};          // 应用关闭标志

            LikesProgram::Net::Address m_remoteAddr;        // 对端地址
            LikesProgram::Net::Address m_localAddr;         // 本端地址
        };
    }
}
