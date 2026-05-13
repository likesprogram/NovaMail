#include <command/RcptCommand.hpp>
#include <Session.hpp>
#include <Transaction.hpp>
#include <types/Types.hpp>
#include <address/EmailAddress.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <core/Utils.hpp>

namespace NovaMail {
    namespace Smtp {
        static bool HandleRcptToParam(const CommandParam& param, Types::RcptTo& rcptTo, std::string_view rawLine) {
            bool hasAddress = false;
            for (const ValuePart& value : param.values) {
                switch (value.role) {
                case ValueRole::Address: {
                    Address::EmailAddress address;
                    hasAddress = Address::EmailAddress::Parse(value.text, address);
                    if (hasAddress) rcptTo.SetAddress(address);
                } break;
                case ValueRole::DisplayName:
                    rcptTo.SetDisplayName(value.text); break;
                default: LogWarn(
                        u"Unexpected value role '{}' in MAIL FROM parameter, command line: {}",
                        std::to_string(static_cast<int>(value.role)), std::string(rawLine)
                    ); break;
                }
            }
            if (!hasAddress) {
                LogWarn(u"MAIL FROM parameter missing address, command line: {}", std::string(rawLine));
                return false;
            }
            return true;
        }

        void RcptCommand::Handle(const std::shared_ptr<Session>& session, const Command& command) {
            if (!session) return;
            // 获取当前状态
            Types::State state = session->GetState();
            
            // 未完成问候/识别/未完成 MAIL
            if (state == Types::State::Connected || state == Types::State::Greeted || state == Types::State::Identified) {
                LogWarn(u"Received RCPT command before EHLO/HELO or MAIL FROM.");
                session->Reply(Types::ReplyCode::BadSequence, "Bad sequence of commands, send HELO/EHLO or MAIL FROM first");
                return;
            }

            // 服务器开启认证，但客户端未认证
            if (session->RelayPolicy() && !session->Authenticated()) {
                LogWarn(u"RCPT command rejected: authentication required, command line: {}", std::string(command.RawLine()));
                session->Reply(Types::ReplyCode::AuthenticationRequired, "Authentication required");
                return;
            }
        
            // 记录 RCPT 参数（收件人地址）
            bool hasTo = false;
            Types::RcptTo rcptTo;
            for (const CommandParam& param : command.Params()) {
                if (param.kind != ParamKind::KeyValue) { // 其他 扩展参数， 先丢弃
                    LogWarn(
                        u"Unexpected positional parameter '{}' in RCPT command, command line: {}",
                        param.raw, std::string(command.RawLine())
                    ); continue;
                }

                // 只找 TO 参数
                if (Core::Utils::IEquals(param.key, "TO")) {
                    if (!HandleRcptToParam(param, rcptTo, command.RawLine())) {
                        LogError(u"Invalid RCPT TO parameter, command line: {}", std::string(command.RawLine()));
                        session->Reply(Types::ReplyCode::ParamError, "Syntax error in parameters or arguments");
                        return;
                    }
                    hasTo = true;
                    continue;
                }

                // TODO: 支持 扩展参数
            }

            // RCPT TO 没出现过
            if (!hasTo) {
                LogError(u"RCPT command missing TO parameter, command line: {}", std::string(command.RawLine()));
                session->Reply(Types::ReplyCode::ParamError, "RCPT command requires TO parameter");
                return;
            }

            // 验证 邮箱
            const bool recipientIsLocal = m_mailBoxService.IsLocalDomain(rcptTo.Address().GetDomain());
            if (!session->Authenticated() && !recipientIsLocal) {
                // 未认证客户端只能投递到本地域名
                LogWarn(
                    u"RCPT TO relay denied: recipient={} client={} -> server={}",
                    rcptTo.Address().ToString(), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                );
                session->Reply(Types::ReplyCode::RelayDenied, "<" + rcptTo.Address().ToString() + ">: Relay access denied");
                return;
            }

            if (recipientIsLocal) {
                NovaMail::Smtp::Entity::MailBox mailBox;
                Service::MailBoxService::GetMailBoxResult getMailBoxResult = m_mailBoxService.GetMailBox(rcptTo.Address(), mailBox);
                switch (getMailBoxResult) {
                case Service::MailBoxService::GetMailBoxResult::NotOwned: // 不关心是否属于当前登录用户，当作成功处理
                case Service::MailBoxService::GetMailBoxResult::Normal: {
                    rcptTo.SetMailBox(mailBox);
                    rcptTo.SetTargetType(Types::RcptTargetType::Local); // 标记
                } break;
                case Service::MailBoxService::GetMailBoxResult::Disabled: { // 邮箱不可用
                    LogWarn(
                        u"RCPT TO rejected: mailbox {} disabled or user inactive, client={} -> server={}",
                        rcptTo.Address().ToString(), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                    );
                    session->Reply(Types::ReplyCode::MailboxUnavailable, "<" + rcptTo.Address().ToString() + ">: Recipient address rejected: Mailbox disabled");
                    return;
                }
                case Service::MailBoxService::GetMailBoxResult::NotFound:{
                    LogWarn(u"RCPT TO rejected: mailbox {} not found, client={} -> server={}",
                        rcptTo.Address().ToString(), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                    );
                    session->Reply(Types::ReplyCode::MailboxUnavailable, "<" + rcptTo.Address().ToString() + ">: Recipient address rejected: Mailbox unknown");
                    return;
                }
                case Service::MailBoxService::GetMailBoxResult::Error:
                default:
                    LogError(
                        u"RCPT TO internal error: mailbox {}, client={} -> server={}",
                        rcptTo.Address().ToString(), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                    );
                    session->Reply(Types::ReplyCode::LocalError, "Internal server error");
                    return;
                }
            } else { // 非本地域名收件人, 需要出站，后续加入事务，由投递模块外发
                LogDebug(
                    u"RCPT TO accepted as outbound recipient: recipient={} authenticatedUser={}",
                    rcptTo.Address().ToString(), session->GetUser() ? session->GetUser()->account : ""
                );
                rcptTo.SetTargetType(Types::RcptTargetType::Outbound); // 标记
            }
            
            Transaction& transaction = session->GetTransaction();
            if (!transaction.AddRcptTo(rcptTo)) {
                LogWarn(u"Add RCPT TO failure: mailbox {} not found, client={} -> server={}",
                    rcptTo.Address().ToString(), session->GetRemoteAddress().ToString(), session->GetLocalAddress().ToString()
                );
            }
            session->SetState(Types::State::RcptCollecting); // 切换状态
            session->Reply(Types::ReplyCode::Ok, "OK");
        }
    }
}