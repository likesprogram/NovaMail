#pragma once
#include <command/CommandHandler.hpp>

namespace NovaMail {
    namespace Imap {
        class LogoutCommand final : public AnyStateCommandHandler {
        public:
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;
        };

    }
}