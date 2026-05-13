#pragma once
#include <command/Command.hpp>
#include <LikesProgram/Configuration.hpp>
#include <LikesProgram/net/Address.hpp>
#include <command/CommandHandler.hpp>
#include <service/UserService.hpp>
#include <service/MailFolderService.hpp>
#include <optional>
#include <array>
#include <memory>
#include <atomic>

namespace NovaMail {
    namespace Imap {
        class Connection;
        enum class Capability : uint64_t {
            None        = 0,

            IMAP4rev2   = 1ull << 0,
            STARTTLS    = 1ull << 1,

            AUTH        = 1ull << 2,
            AUTH_PLAIN  = 1ull << 3,
            AUTH_LOGIN  = 1ull << 4,

            IDLE        = 1ull << 5,
            MOVE        = 1ull << 6,
            NAMESPACE   = 1ull << 7,
            UNSELECT    = 1ull << 8,
            ENABLE      = 1ull << 9,

            LITERAL_PLUS = 1ull << 10
        };

        inline Capability operator|(Capability lhs, Capability rhs) {
            return static_cast<Capability>(
                static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)
            );
        }

        inline Capability& operator|=(Capability& lhs, Capability rhs) {
            lhs = lhs | rhs;
            return lhs;
        }

        inline bool HasCapability(Capability caps, Capability cap) {
            return (
                static_cast<uint64_t>(caps) &
                static_cast<uint64_t>(cap)
            ) != 0;
        }
        enum class State {
            NotAuthenticated, // 未登录
            Authenticated,    // 已登录，但未选择邮箱
            Selected,         // 已选择邮箱
            Logout            // 准备关闭
        };
        enum class Status {
            Ok,
            No,
            Bad
        };
        class Session : public std::enable_shared_from_this<Session> {
        public:
            Session(Connection* connection, LikesProgram::Net::Address remoteAddr, LikesProgram::Net::Address localAddr);
            ~Session() = default;

            // 初始化处理类
            void InitHandlers();

            void OnConnect(); // 连接成功
            void OnBytes(LikesProgram::Net::Buffer& in); // 处理输入
            void OnTlsHandshakeDone(); // 握手完成
            bool ShouldClose(bool value = false, bool set = false) noexcept;           // 应用关闭

            void ReplyTagged(const std::string& tag, Status status, const std::string& message);
            void ReplyUntagged(const std::string& message);
            void ReplyContinuation(const std::string& message);
    
            // 切换状态
            void SetState(State state);
            // 获取当前状态
            State GetState() const noexcept;

            // 开启 TLS 对话
            void StartTls();
            // 当前是否已经是 SSL/TLS 模式
            bool IsTLS();

            // 保存登录的用户
            void SetUser(const Entity::User& user);
            // 已认证
            bool Authenticated() const noexcept;
            const Entity::User* GetUser() const noexcept;
            Capability GetCapabilities() const noexcept;

            // 选择当前会话要操作的邮箱，并设置是否为只读模式
            void SelectMailbox(const Service::MailFolderInfo& mailbox, bool readOnly, uint64_t exists);
            uint64_t GetSelectedExists() const noexcept;
            void UpdateSelectedExists(uint64_t exists) noexcept;
            // 清除当前会话已选择的邮箱状态
            void ClearSelectedMailbox();
            // 判断当前会话是否已经选择邮箱
            bool HasSelectedMailbox() const noexcept;
            // 获取当前会话已选择的邮箱信息，未选择时返回 nullptr
            const Service::MailFolderInfo* GetSelectedMailbox() const noexcept;
            // 判断当前已选择的邮箱是否为只读模式
            bool SelectedReadOnly() const noexcept;

            // 获取 Smtp 服务名称
            std::string ImapHostname() const noexcept;

            // 切换为行输入模式
            void BeginActiveLine(CommandVerb verb, std::string tag);
            // 切换为字节输入模式
            void BeginLiteralBytes(CommandVerb verb, std::string tag);
            // 获取正在处理的指令TAG
            std::string_view ActiveTag() const noexcept;
            // 指令处理完成
            void FinishActiveCommand();

            // 获取对端地址
            const LikesProgram::Net::Address& GetRemoteAddress() const noexcept;
            // 获取本段地址
            const LikesProgram::Net::Address& GetLocalAddress() const noexcept;
        private:
            // 尝试解析指令
            std::optional<Command> TryParseCommand(LikesProgram::Net::Buffer& in);

            // 行级模式派遣处理
            void DispatchActiveLine(const Command& line);
            // 指令派遣处理
            void Dispatch(const Command& command);

            // 发送返回值
            void SendRaw(const std::string& data);
        private:
            Connection* m_connection = nullptr;             // 服务控制器
            std::array<std::unique_ptr<CommandHandler>, static_cast<size_t>(CommandVerb::COUNT)> m_handlers; // 指令处理器
            State m_state = State::NotAuthenticated;        // 当前 IMAP 状态机状态
            LikesProgram::Configuration m_config;           // 配置
            std::string m_imapHostname = "NovaMail";        // IMAP 服务名称
            Capability m_caps = Capability::None;           // 服务能力
            InputMode m_inputMode = InputMode::CommandLine; // 指令输入模式
            CommandVerb m_activeVerb = CommandVerb::Unknown;// 正在执行的指令动词
            std::string m_activeTag;                        // 当前正在处理的指令

            std::optional<Service::MailFolderInfo> m_selectedMailbox;
            bool m_selectedReadOnly{false};
            uint64_t m_selectedExists{0};

            bool             m_isTLS = false;               // 当前是否启用 SLL/TLS

            std::optional<Entity::User> m_authUser;         // 认证的用户

            std::atomic_bool m_shouldClose{false};          // 应用关闭标志

            LikesProgram::Net::Address m_remoteAddr;        // 对端地址
            LikesProgram::Net::Address m_localAddr;         // 本端地址
        };
    }
}
