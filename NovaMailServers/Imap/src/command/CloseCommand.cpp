#include <command/CloseCommand.hpp>
#include <Session.hpp>
#include <command/Command.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Imap {

        void CloseCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;

            const std::string tag(command.Tag());

            if (!command.Arguments().empty()) {
                session->ReplyTagged(tag, Status::Bad, "CLOSE takes no arguments");
                return;
            }

            if (!session->HasSelectedMailbox()) {
                session->ReplyTagged(tag, Status::Bad, "No mailbox selected");
                return;
            }

            const auto* selected = session->GetSelectedMailbox();

            LogInfo(
                u"IMAP CLOSE mailbox: mailbox={} folderId={} client={} -> server={}",
                selected ? selected->imapName : "",
                selected ? static_cast<int64_t>(selected->folderId) : 0,
                session->GetRemoteAddress().ToString(),
                session->GetLocalAddress().ToString()
            );

            session->ReplyTagged(tag, Status::Ok, "CLOSE completed");
        }

    }
}