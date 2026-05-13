#pragma once
#include <command/CommandHandler.hpp>
#include <service/MailBoxService.hpp>
#include <memory>
#include <string>

namespace NovaMail {
    namespace Imap {
        class CheckCommand final : public SelectedCommandHandler {
        public:
            CheckCommand(): m_mailBoxService(LikeHub::Storage::DatabasePool::Instance()) { }
            void Handle(const std::shared_ptr<Session>& session, const Command& command) override;
        private:
            Service::MailBoxService m_mailBoxService;
        };

    }
}