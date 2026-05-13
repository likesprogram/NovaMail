#include <command/CommandHandler.hpp>

namespace NovaMail {
    namespace Smtp {
        CommandHandler::DataResult CommandHandler::InputData(const std::shared_ptr<Session>& session, LikesProgram::Net::Buffer& in) {
            (void) session;
            (void) in;
            return DataResult::TransactionFinished;
        }
    }
}