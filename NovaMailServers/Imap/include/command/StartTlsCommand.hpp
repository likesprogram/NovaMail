#pragma once
#include <command/CommandHandler.hpp>

namespace NovaMail {
    namespace Imap {
        class StartTlsCommand : public NotAuthenticatedCommandHandler {
        public:
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;
        };

    }
}