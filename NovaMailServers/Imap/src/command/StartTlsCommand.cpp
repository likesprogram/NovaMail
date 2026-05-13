#include <command/StartTlsCommand.hpp>
#include <command/Command.hpp>
#include <Session.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Imap {
        void StartTlsCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;

            const std::string tag = std::string(command.Tag());

            // 已经 TLS
            if (session->IsTLS()) {
                session->ReplyTagged(tag, Status::Bad, "TLS already active");
                return;
            }

            // 登录后不允许再 STARTTLS
            if (session->GetState() != State::NotAuthenticated) {
                session->ReplyTagged(tag, Status::Bad, "STARTTLS not allowed in current state");
                return;
            }

            session->ReplyTagged(tag, Status::Ok, "Begin TLS negotiation now");

            LogInfo(
                u"IMAP STARTTLS accepted: client={} -> server={}",
                session->GetRemoteAddress().ToString(),
                session->GetLocalAddress().ToString()
            );

            session->StartTls();
        }

    }
}