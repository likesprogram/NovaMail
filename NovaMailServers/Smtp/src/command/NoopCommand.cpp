#include <command/NoopCommand.hpp>
#include <Session.hpp>
#include <Transaction.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Smtp {
        void NoopCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;
            (void) command;
            session->Reply(Types::ReplyCode::Ok, "OK");
        }
    }
}