#include <command/EhloCommand.hpp>
#include <Session.hpp>
#include <Transaction.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace NovaMail {
    namespace Smtp {
        EhloCommand::EhloCommand(std::string hostname, std::string greeting) : m_hostname(hostname), m_greeting(greeting) { }
        
        void EhloCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;
            std::string message = "";
            message.append(m_hostname);
            message.append(" ");
            message.append(m_greeting);
            message.append("\r\n");

            Transaction& transaction = session->GetTransaction();
            transaction.Reset(); // 先清空 内容，防止污染
            Types::Capabilities caps = session->GetCapabilities();
            if (caps != Types::Capabilities::None) {
                if(Types::HasCapability(caps, Types::Capabilities::ESMTP)) {
                    message.append("250-ESMTP\r\n");
                }
                if(Types::HasCapability(caps, Types::Capabilities::PIPELINING)) {
                    message.append("250-PIPELINING\r\n");
                }
                if(Types::HasCapability(caps, Types::Capabilities::SIZE)) {
                    message.append("250-SIZE ");
                    message.append(std::to_string(transaction.MaxDataBytes()));
                    message.append("\r\n");
                }
                if (Types::HasCapability(caps, Types::Capabilities::ENHANCED_STATUS)) {
                    message.append("250-ENHANCEDSTATUSCODES\r\n");
                }
                if (Types::HasCapability(caps, Types::Capabilities::DSN)) {
                    message.append("250-DSN\r\n");
                }
                if(Types::HasCapability(caps, Types::Capabilities::STARTTLS) && !session->IsTLS()) {
                    message.append("250-STARTTLS\r\n");
                }
                if (Types::HasCapability(caps, Types::Capabilities::AUTH)) {
                    message.append("250-AUTH");
                    if (Types::HasCapability(caps, Types::Capabilities::AUTH_PLAIN)) message.append(" PLAIN");
                    if (Types::HasCapability(caps, Types::Capabilities::AUTH_LOGIN)) message.append(" LOGIN");
                    // if (Types::HasCapability(caps, Types::Capabilities::AUTH_CRAM_MD5)) message.append(" CRAM-MD5");
                    message.append("\r\n");
                }
                if (Types::HasCapability(caps, Types::Capabilities::EIGHT_BIT_MIME)) {
                    message.append("250-8BITMIME\r\n");
                }
                if (Types::HasCapability(caps, Types::Capabilities::BINARY_MIME)) {
                    message.append("250-BINARYMIME\r\n");
                }
                if (Types::HasCapability(caps, Types::Capabilities::CHUNKING)) {
                    message.append("250-CHUNKING\r\n");
                }
                if (Types::HasCapability(caps, Types::Capabilities::SMTP_UTF8)) {
                    message.append("250-SMTPUTF8\r\n");
                }
                if (Types::HasCapability(caps, Types::Capabilities::VRFY)) {
                    message.append("250-VRFY\r\n");
                }
                if (Types::HasCapability(caps, Types::Capabilities::EXPN)) {
                    message.append("250-EXPN\r\n");
                }
                if (Types::HasCapability(caps, Types::Capabilities::HELP)) {
                    message.append("250-HELP\r\n");
                }
            }

            // 记录 EHLO 参数（客户端域名）
            if (command.Params().size() > 0 && command.Params().front().values.size() > 0) transaction.SetEhloDomain(command.Params().front().values.front().text);

            // 回复 250，EHLO 完成
            message.append("250 OK");

            // 输出日志
            LogInfo(
                u"EHLO from clientDomain={}, client={} -> server={}",
                transaction.HasEhloDomain() ? std::string(transaction.EhloDomain()) : std::string("unknown"),
                session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
            );
            
            session->SetState(Types::State::Identified); // EHLO/HELO 完成
            session->Reply(Types::ReplyCode::Ok, message, "-");
        }
    }
}