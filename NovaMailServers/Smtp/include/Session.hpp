#pragma once
#include <Types.hpp>
#include <Command.hpp>
#include <Transaction.hpp>
#include <LikesProgram/net/Connection.hpp>
#include <LikesProgram/Configuration.hpp>
#include <optional>

namespace NovaMail {
    namespace Smtp {
        class Session {
        public:
            Session();
            ~Session() = default;
            void OnConnect(LikesProgram::Net::Buffer& out); // 发送 220
            void OnBytes(LikesProgram::Net::Buffer& in, LikesProgram::Net::Buffer& out); // 处理输入
            bool ShouldClose() const noexcept;              // 应用关闭
        private:
            // 指令处理
            void OnEhlo(const Command& command, LikesProgram::Net::Buffer& out);
            void OnMail(const Command& command, LikesProgram::Net::Buffer& out);
            void OnRcpt(const Command& command, LikesProgram::Net::Buffer& out);
            void OnData(LikesProgram::Net::Buffer& out);
            void OnRset(LikesProgram::Net::Buffer& out);
            void OnNoop(LikesProgram::Net::Buffer& out);
            void OnQuit(LikesProgram::Net::Buffer& out);

            // 指令拼写错误或不认识
            void Reply500(LikesProgram::Net::Buffer& out);

            // 指令参数错误
            void Reply501(LikesProgram::Net::Buffer& out);
            
            // 认识但是不支持该指令
            void Reply502(LikesProgram::Net::Buffer& out);
            
            // 指令顺序错误
            void Reply503(LikesProgram::Net::Buffer& out);

            // 处理数据
            void HandleData(LikesProgram::Net::Buffer& in, LikesProgram::Net::Buffer& out);

            // 推进数据处理
            bool PumpData(Buffer& in, Buffer& out);

            // 尝试解析指令
            std::optional<Command> TryParseCommand(LikesProgram::Net::Buffer& in);

            // 指令派遣处理
            void Dispatch(const Command& command, LikesProgram::Net::Buffer& out);
        private:
            State            m_state = State::Connected;    // 当前 SMTP 状态机状态
            LikesProgram::Configuration m_config;           // 配置
            Transaction      m_tx;                          // 当前邮件事务容器
            Capabilities     m_caps;                        // 当前会话支持的能力集合
            bool             m_isTLS;                       // 当前是否启用 SLL/TLS
            bool             m_shouldClose = false;         // 关闭标志
        };
    }
}
