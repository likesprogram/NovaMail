#include <command/LogoutCommand.hpp>
#include <command/Command.hpp>
#include <Session.hpp>
#include <memory>

namespace NovaMail {
    namespace Imap {
        static bool HasArguments(const Command& command) {
            const std::string args = std::string(command.Arguments());
            return args.find_first_not_of(" \t") != std::string::npos;
        }

        void LogoutCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;

            const std::string tag = std::string(command.Tag());

            // LOGOUT 不允许携带参数
            if (HasArguments(command)) {
                session->ReplyTagged(tag, Status::Bad, "LOGOUT takes no arguments");
                return;
            }

            session->ReplyUntagged("BYE " + session->ImapHostname() + " IMAP Service logging out");
            session->ReplyTagged(tag, Status::Ok, "LOGOUT completed");

            // 关闭连接
            session->ShouldClose(true, true);;
        }

    }
}