#include <command/NoopCommand.hpp>
#include <command/Command.hpp>
#include <Session.hpp>
#include <memory>

namespace NovaMail {
    namespace Imap {
        static bool HasArguments(const Command& command) {
            const std::string args = std::string(command.Arguments());
            return args.find_first_not_of(" \t") != std::string::npos;
        }

        void NoopCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;
            const std::string tag = std::string(command.Tag());

            // NOOP 不允许携带参数
            if (HasArguments(command)) {
                session->ReplyTagged(tag, Status::Bad, "NOOP takes no arguments");
                return;
            }

            session->ReplyTagged(tag, Status::Ok, "NOOP completed");
        }

    }
}