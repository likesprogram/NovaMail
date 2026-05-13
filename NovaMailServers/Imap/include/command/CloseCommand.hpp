#pragma once

#include <command/CommandHandler.hpp>
#include <memory>
#include <string>

namespace NovaMail {
    namespace Imap {
        class CloseCommand final : public SelectedCommandHandler {
        public:
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;
        };

    }
}