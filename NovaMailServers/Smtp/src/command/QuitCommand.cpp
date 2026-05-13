#include <command/QuitCommand.hpp>
#include <Session.hpp>
#include <Transaction.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Smtp {
        void QuitCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;
            (void) command;
            
            // 先发送 再关闭服务
            session->Reply(Types::ReplyCode::ClosingChannel, "Nova Mail Service closing transmission channel");

            // 输出日志
            Transaction& transaction = session->GetTransaction();
            LogInfo(
                u"SMTP client quit: clientDomain={}, client={} -> server={}",
                transaction.HasEhloDomain() ? std::string(transaction.EhloDomain()) : std::string("unknown"),
                session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
            );
            session->ShouldClose(true, true); // 关闭服务
        }
    }
}