#include <command/CommandHandler.hpp>
#include <command/Command.hpp>
#include <Session.hpp>

namespace NovaMail {
    namespace Imap {
        InputResult CommandHandler::InputLine(const std::shared_ptr<Session>& session, const Command& line) {
            (void)line;
            if (session) {
                const std::string tag = std::string(session->ActiveTag().empty() ? "*" : session->ActiveTag());
                session->ReplyTagged(tag, Status::Bad, "Unexpected continuation data");
            }
            return InputResult::Failed;
        }
        InputResult CommandHandler::InputData(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in) {
            (void)in;
            if (session) {
                const std::string tag = std::string(session->ActiveTag().empty() ? "*" : session->ActiveTag());
                session->ReplyTagged(tag, Status::Bad, "Unexpected continuation data");
            }
            return InputResult::Failed;
        }
        
        bool AnyStateCommandHandler::IsAllowed(const Session& session, const Command& command) const {
            (void)session; (void)command;
            return true;
        }
    
        bool NotAuthenticatedCommandHandler::IsAllowed(const Session& session, const Command& command) const {
            (void)command;
            return session.GetState() == State::NotAuthenticated;
        }
    
        bool AuthenticatedCommandHandler::IsAllowed(const Session& session, const Command& command) const {
            (void)command;
            return session.GetState() == State::Authenticated || session.GetState() == State::Selected;
        }
    
        bool SelectedCommandHandler::IsAllowed(const Session& session, const Command& command) const {
            (void)command;
            return session.GetState() == State::Selected;
        }
    }
}