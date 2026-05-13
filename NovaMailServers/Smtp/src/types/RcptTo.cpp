#include <types/RcptTo.hpp>

namespace NovaMail {
    namespace Smtp::Types {
        RcptTo::RcptTo() { Reset(); }

        Entity::MailBox RcptTo::MailBox() const noexcept {
            return m_mailbox;
        }

        Address::EmailAddress RcptTo::Address() const noexcept {
            return m_address;
        }

        std::string_view RcptTo::DisplayName() const noexcept {
            return m_displayName;
        }

        RcptNotify RcptTo::Notify() const noexcept {
            return m_notify;
        }

        std::string_view RcptTo::OrCpt() const noexcept {
            return m_orcpt;
        }

        const std::unordered_map<std::string,std::string>& RcptTo::Extensions() const noexcept {
            return m_extensions;
        }
        
        bool RcptTo::IsLocalTarget() const noexcept {
            return m_targetType == RcptTargetType::Local;
        }
    
        bool RcptTo::IsOutboundTarget() const noexcept {
            return m_targetType == RcptTargetType::Outbound;
        }

        void RcptTo::SetMailBox(const Entity::MailBox& mailbox) {
            m_mailbox = mailbox;
        }

        void RcptTo::SetAddress(const Address::EmailAddress& address) {
            m_address = address;
            m_addressString = address.ToString();
        }

        void RcptTo::SetDisplayName(std::string_view displayName) {
            m_displayName.assign(displayName.data(), displayName.size());
        }

        void RcptTo::SetNotify(RcptNotify notify) noexcept {
            m_notify = notify;
        }

        void RcptTo::AddNotify(RcptNotify notify) noexcept {
            m_notify = static_cast<RcptNotify>( static_cast<uint8_t>(m_notify) | static_cast<uint8_t>(notify));
        }

        void RcptTo::SetOrCpt(std::string_view orcpt) {
            m_orcpt.assign(orcpt.data(), orcpt.size());
        }

        void RcptTo::SetExtension(std::string_view key,std::string_view value) {
            m_extensions.emplace(std::string(key), std::string(value));
        }

        void RcptTo::SetTargetType(RcptTargetType type) {
            m_targetType = type;
        }

        static std::string GetStringOrDefault(
            const LikesProgram::Configuration::ObjectMap& obj,
            const LikesProgram::String& key,
            const std::string& defaultValue = ""
        ) {
            auto it = obj.find(key);

            if (it == obj.end() || it->second.IsNull()) {
                return defaultValue;
            }

            return it->second
                .AsString(LikesProgram::Configuration::CastPolicy::AutoConvert)
                .ToStdString();
        }

        static int64_t GetInt64OrDefault(
            const LikesProgram::Configuration::ObjectMap& obj,
            const LikesProgram::String& key,
            int64_t defaultValue = 0
        ) {
            auto it = obj.find(key);

            if (it == obj.end() || it->second.IsNull()) {
                return defaultValue;
            }

            return it->second.AsInt64(
                LikesProgram::Configuration::CastPolicy::AutoConvert
            );
        }

        static std::unordered_map<std::string, std::string> GetStringMapOrDefault(
            const LikesProgram::Configuration::ObjectMap& obj,
            const LikesProgram::String& key
        ) {
            std::unordered_map<std::string, std::string> result;

            auto it = obj.find(key);

            if (it == obj.end() || it->second.IsNull()) {
                return result;
            }

            if (!it->second.IsObject()) {
                throw std::runtime_error(
                    "RcptTo field '" + key.ToStdString() + "' must be object"
                );
            }

            const auto& extObj = it->second.AsObject();

            for (const auto& [extKey, extValue] : extObj) {
                result.emplace(
                    extKey.ToStdString(),
                    extValue
                        .AsString(LikesProgram::Configuration::CastPolicy::AutoConvert)
                        .ToStdString()
                );
            }

            return result;
        }

        LikesProgram::Configuration RcptTo::ToConfiguration() const {
            LikesProgram::Configuration cfg;

            cfg[u"mailbox"] = m_mailbox.ToConfiguration();

            if (HasAddress()) {
                cfg[u"address"] = m_address.ToConfiguration();
            }

            cfg[u"addressString"] = LikesProgram::String(m_addressString);
            cfg[u"displayName"] = LikesProgram::String(m_displayName);

            cfg[u"notify"] = static_cast<int64_t>(m_notify);
            cfg[u"orcpt"] = LikesProgram::String(m_orcpt);

            LikesProgram::Configuration extensions;

            for (const auto& [key, value] : m_extensions) {
                extensions[LikesProgram::String(key)] = LikesProgram::String(value);
            }

            cfg[u"extensions"] = extensions;
            cfg[u"targetType"] = static_cast<int64_t>(m_targetType);

            return cfg;
        }

        RcptTo RcptTo::FromConfiguration(const LikesProgram::Configuration& cfg) {
            if (!cfg.IsObject()) {
                throw std::runtime_error("RcptTo configuration must be object");
            }

            const auto& obj = cfg.AsObject();

            RcptTo rcpt;

            auto mailboxIt = obj.find(u"mailbox");

            if (mailboxIt != obj.end() && !mailboxIt->second.IsNull()) {
                rcpt.m_mailbox = Entity::MailBox::FromConfiguration(mailboxIt->second);
            }

            rcpt.m_displayName = GetStringOrDefault(
                obj,
                u"displayName",
                ""
            );

            rcpt.m_notify = static_cast<RcptNotify>(
                GetInt64OrDefault(
                    obj,
                    u"notify",
                    static_cast<int64_t>(RcptNotify::None)
                )
            );

            rcpt.m_orcpt = GetStringOrDefault(
                obj,
                u"orcpt",
                ""
            );

            rcpt.m_extensions = GetStringMapOrDefault(
                obj,
                u"extensions"
            );

            rcpt.m_targetType = static_cast<RcptTargetType>(
                GetInt64OrDefault(
                    obj,
                    u"targetType",
                    static_cast<int64_t>(RcptTargetType::Unknown)
                )
            );

            /*
             * 优先使用 address 字段。
             * 如果没有 address，则使用 addressString 解析。
             */
            auto addressIt = obj.find(u"address");

            if (addressIt != obj.end() && !addressIt->second.IsNull()) {
                rcpt.m_address = Address::EmailAddress::FromConfiguration(addressIt->second);
                rcpt.m_addressString = rcpt.m_address.ToString();
            } else {
                rcpt.m_addressString = GetStringOrDefault(
                    obj,
                    u"addressString",
                    ""
                );

                if (!rcpt.m_addressString.empty()) {
                    Address::EmailAddress parsedAddress;

                    if (!Address::EmailAddress::Parse(rcpt.m_addressString, parsedAddress)) {
                        throw std::runtime_error(
                            "Invalid RcptTo addressString: " + rcpt.m_addressString
                        );
                    }

                    rcpt.m_address = parsedAddress;
                }
            }

            /*
             * RCPT TO 必须有收件人地址。
             */
            if (rcpt.m_addressString.empty()) {
                throw std::runtime_error("RcptTo address is empty");
            }

            return rcpt;
        }

        bool RcptTo::ParseConfiguration(
            const LikesProgram::Configuration& cfg,
            RcptTo& out
        ) noexcept {
            try {
                out = RcptTo::FromConfiguration(cfg);
                return true;
            } catch (...) {
                out.Reset();
                return false;
            }
        }

        void RcptTo::Reset() {
            m_address.Reset();
            m_addressString.clear();
            m_displayName.clear();
            m_orcpt.clear();

            m_notify = RcptNotify::None;

            m_extensions.clear();
        }
    }
}