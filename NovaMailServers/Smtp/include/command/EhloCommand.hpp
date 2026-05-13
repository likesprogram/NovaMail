#pragma once
#include <command/CommandHandler.hpp>
#include <types/Types.hpp>
#include <string>

namespace NovaMail {
    namespace Smtp {
        class EhloCommand: public CommandHandler {
        public:
            EhloCommand(std::string hostname, std::string greeting);
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;
        private:
            std::string m_hostname;
            std::string m_greeting;
        };
    }
}