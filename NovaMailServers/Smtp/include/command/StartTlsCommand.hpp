#pragma once
#include <command/CommandHandler.hpp>

namespace NovaMail {
    namespace Smtp {
        class StartTlsCommand : public CommandHandler {
        public:
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;
        };
    }
}