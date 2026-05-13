#include <command/CapabilityCommand.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <command/Command.hpp>
#include <Session.hpp>

namespace NovaMail {
    namespace Imap {
        void CapabilityCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;
            std::string message = "CAPABILITY";
            Capability caps = session->GetCapabilities();
            if (HasCapability(caps, Capability::IMAP4rev2)) message.append(" IMAP4rev2");
            if (HasCapability(caps, Capability::LITERAL_PLUS)) message.append(" LITERAL+");
            // STARTTLS 只在 TLS 前声明， TLS 建立后不能继续声明 STARTTLS
            if (HasCapability(caps, Capability::STARTTLS) && !session->IsTLS()) message.append(" STARTTLS");
            // 如果已登录，通常没必要继续声明 AUTH
            if (!session->Authenticated() && HasCapability(caps, Capability::AUTH)) {
                if (HasCapability(caps, Capability::AUTH_PLAIN)) message.append(" AUTH=PLAIN");
                if (HasCapability(caps, Capability::AUTH_LOGIN)) message.append(" AUTH=LOGIN");
            }
            if (HasCapability(caps, Capability::IDLE)) message.append(" IDLE");
            if (HasCapability(caps, Capability::MOVE)) message.append(" MOVE");
            if (HasCapability(caps, Capability::NAMESPACE)) message.append(" NAMESPACE");
            if (HasCapability(caps, Capability::UNSELECT)) message.append(" UNSELECT");
            if (HasCapability(caps, Capability::ENABLE)) message.append(" ENABLE");

            LogInfo(
                u"IMAP CAPABILITY requested: client={} -> server={}, caps={}",
                session->GetRemoteAddress().ToString(),
                session->GetLocalAddress().ToString(),
                LikesProgram::String(message)
            );

            session->ReplyUntagged(message);
            session->ReplyTagged(std::string(command.Tag()), Status::Ok, "CAPABILITY completed");
        }
    }
}