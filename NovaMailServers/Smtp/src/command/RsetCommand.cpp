#include <command/RsetCommand.hpp>
#include <Session.hpp>
#include <Transaction.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Smtp {
        void RsetCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;
            (void) command;
            session->GetTransaction().Reset(); // 清理缓存

            // 重置状态
            if (session->GetState() >= Types::State::Identified) session->SetState(Types::State::Identified);
            else session->SetState(Types::State::Greeted);
            
            // 返回响应
            session->Reply(Types::ReplyCode::Ok, "OK");
        }
    }
}