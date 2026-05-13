#pragma once
#include <command/CommandHandler.hpp>
#include <types/Types.hpp>
#include <string>

namespace NovaMail {
    namespace Smtp {
        class NoopCommand: public CommandHandler {
        public:
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;
        };
    }
}