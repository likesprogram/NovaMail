#pragma once
#include <command/CommandHandler.hpp>
#include <service/MailBoxService.hpp>
#include <types/Types.hpp>
#include <string>

namespace NovaMail {
    namespace Smtp {
        class RcptCommand: public CommandHandler {
        public:
            RcptCommand(): m_mailBoxService(LikeHub::Storage::DatabasePool::Instance()) { }
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;
        private:
            NovaMail::Smtp::Service::MailBoxService m_mailBoxService;
        };
    }
}