#include <command/StartTlsCommand.hpp>
#include <Session.hpp>
#include <Connection.hpp>

namespace NovaMail {
    namespace Smtp {
        void StartTlsCommand::Handle(const std::shared_ptr<Session>& session, const Command&) {
            if (!session) return;

            // 已经 TLS
            if (session->IsTLS()) {
                session->Reply(Types::ReplyCode::BadSequence, "TLS already active");
                return;
            }

            // RFC3207:
            // STARTTLS 必须在 MAIL 事务前
            auto state = session->GetState();
            if (state == Types::State::MailStarted || state == Types::State::RcptCollecting || state == Types::State::DataReceiving || state == Types::State::BdatReceiving) {
                session->Reply(Types::ReplyCode::BadSequence, "STARTTLS not allowed during mail transaction");
                return;
            }
            session->Reply(Types::ReplyCode::ServiceReady, "Ready to start TLS");
            session->StartTls();
        }
    }
}