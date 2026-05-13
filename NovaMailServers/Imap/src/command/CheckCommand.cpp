#include <command/CheckCommand.hpp>
#include <Session.hpp>
#include <command/Command.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Imap {
        void CheckCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;
            const std::string tag(command.Tag());

            if (!command.Arguments().empty()) {
                session->ReplyTagged(tag, Status::Bad, "CHECK takes no arguments");
                return;
            }

            const auto* user = session->GetUser();
            if (!user || user->mailBoxId == 0) {
                session->ReplyTagged(tag, Status::No, "No available mailbox");
                return;
            }
        
            const auto* selected = session->GetSelectedMailbox();
            if (!selected || !selected->Valid()) {
                session->ReplyTagged(tag, Status::Bad, "No mailbox selected");
                return;
            }

            uint64_t messages = 0;
            uint64_t unseen = 0;
            uint64_t recent = 0;
            if (!m_mailBoxService.GetMailboxStatus(user->mailBoxId, selected->folderId, messages, unseen, recent)) {
                session->ReplyTagged(tag, Status::No, "CHECK failed");
                return;
            }

            const uint64_t oldExists = session->GetSelectedExists();
            if (messages > oldExists) {
                session->UpdateSelectedExists(messages);
                session->ReplyUntagged(std::to_string(messages) + " EXISTS");
            } else if (messages < oldExists) {
                LogDebug(
                    u"IMAP CHECK detected message count decreased but EXPUNGE notification is not implemented: oldExists={} newExists={} mailbox={} folderId={}",
                    static_cast<int64_t>(oldExists), static_cast<int64_t>(messages), selected->imapName, static_cast<int64_t>(selected->folderId)
                );
            }

            session->ReplyTagged(tag, Status::Ok, "CHECK completed");

            LogDebug(
                u"IMAP CHECK completed: client={} -> server={}",
                session->GetRemoteAddress().ToString(),
                session->GetLocalAddress().ToString()
            );
        }
    }
}