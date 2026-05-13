#include <types/MailFrom.hpp>
#include <stdexcept>
#include <limits>
#include <string>

namespace NovaMail {
    namespace Smtp::Types {
        MailFrom::MailFrom() { Reset(); }

        Entity::MailBox MailFrom::MailBox() const noexcept {
            return m_mailbox;
        }

        Address::EmailAddress MailFrom::Address() const noexcept {
            return m_address;
        }
        
        std::string_view MailFrom::DisplayName() const noexcept {
            return m_displayName;
        }

        size_t MailFrom::MailSize() const noexcept {
            return m_mailSize;
        }

        MailBody MailFrom::MailBodyType() const noexcept {
            return m_body;
        }
        
        bool MailFrom::NullReversePath() const noexcept {
            return m_nullReversePath;
        }

        bool MailFrom::IsLocalSource() const noexcept {
            return m_sourceType == MailSourceType::Local;
        }
    
        bool MailFrom::IsExternalSource() const noexcept {
            return m_sourceType == MailSourceType::External;
        }
    
        bool MailFrom::IsNullReversePath() const noexcept {
            return m_sourceType == MailSourceType::NullReversePath;
        }

        void MailFrom::SetMailBox(const Entity::MailBox& mailbox) {
            m_mailbox = mailbox;
        }

        void MailFrom::SetAddress(const Address::EmailAddress& address) {
            m_address = address;
            m_addressString = address.ToString();
        }

        void MailFrom::SetDisplayName(std::string_view displayName) {
            m_displayName.assign(displayName.data(), displayName.size());
        }

        void MailFrom::SetSize(size_t mailSize) noexcept {
            m_mailSize = mailSize;
        }

        void MailFrom::SetBody(std::string_view body) {
            m_body = ParseMailBody(body);
        }
        
        void MailFrom::SetNullReversePath(bool value) {
            m_nullReversePath = value;
        }
        
        void MailFrom::SetSourceType(MailSourceType type) {
            m_sourceType = type;
        }

        static std::string GetStringOrDefault(const LikesProgram::Configuration::ObjectMap& obj, const LikesProgram::String& key, const std::string& defaultValue = "") {
            auto it = obj.find(key);

            if (it == obj.end() || it->second.IsNull()) {
                return defaultValue;
            }

            return it->second.AsString(LikesProgram::Configuration::CastPolicy::AutoConvert).ToStdString();
        }

        static bool GetBoolOrDefault(
            const LikesProgram::Configuration::ObjectMap& obj,
            const LikesProgram::String& key,
            bool defaultValue = false
        ) {
            auto it = obj.find(key);

            if (it == obj.end() || it->second.IsNull()) {
                return defaultValue;
            }

            return it->second.AsBool(
                LikesProgram::Configuration::CastPolicy::AutoConvert
            );
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

        static size_t GetSizeTOrDefault(
            const LikesProgram::Configuration::ObjectMap& obj,
            const LikesProgram::String& key,
            size_t defaultValue = 0
        ) {
            auto it = obj.find(key);

            if (it == obj.end() || it->second.IsNull()) {
                return defaultValue;
            }

            const LikesProgram::Configuration& value = it->second;

            if (value.IsString()) {
                std::string text = value.AsString().ToStdString();

                if (text.empty()) {
                    return defaultValue;
                }

                size_t parsedLength = 0;
                unsigned long long parsed = 0;

                try {
                    parsed = std::stoull(text, &parsedLength, 10);
                } catch (...) {
                    throw std::runtime_error(
                        "MailFrom field '" + key.ToStdString() + "' is not valid size_t"
                    );
                }

                if (parsedLength != text.size()) {
                    throw std::runtime_error(
                        "MailFrom field '" + key.ToStdString() + "' contains invalid characters"
                    );
                }

                if (parsed > static_cast<unsigned long long>(std::numeric_limits<size_t>::max())) {
                    throw std::overflow_error(
                        "MailFrom field '" + key.ToStdString() + "' exceeds size_t range"
                    );
                }

                return static_cast<size_t>(parsed);
            }

            int64_t signedValue = value.AsInt64(
                LikesProgram::Configuration::CastPolicy::AutoConvert
            );

            if (signedValue < 0) {
                throw std::runtime_error(
                    "MailFrom field '" + key.ToStdString() + "' cannot be negative"
                );
            }

            return static_cast<size_t>(signedValue);
        }

        LikesProgram::Configuration MailFrom::ToConfiguration() const {
            LikesProgram::Configuration cfg;

            cfg[u"mailbox"] = m_mailbox.ToConfiguration();

            /*
             * address 保存标准 EmailAddress；
             * addressString 保存原始/展示用地址字符串。
             *
             * NullReversePath 情况下 address 可能为空，因此只在有地址时写入。
             */
            if (HasAddress()) {
                cfg[u"address"] = m_address.ToConfiguration();
            }

            cfg[u"addressString"] = LikesProgram::String(m_addressString);
            cfg[u"displayName"] = LikesProgram::String(m_displayName);

            /*
             * size_t 用字符串保存，避免 Configuration 没有 uint64_t 类型导致溢出。
             */
            cfg[u"mailSize"] = LikesProgram::String(std::to_string(m_mailSize));

            cfg[u"body"] = static_cast<int64_t>(m_body);
            cfg[u"sourceType"] = static_cast<int64_t>(m_sourceType);
            cfg[u"nullReversePath"] = m_nullReversePath;

            return cfg;
        }

        MailFrom MailFrom::FromConfiguration(const LikesProgram::Configuration& cfg) {
            if (!cfg.IsObject()) {
                throw std::runtime_error("MailFrom configuration must be object");
            }

            const auto& obj = cfg.AsObject();

            MailFrom mailFrom;

            auto mailboxIt = obj.find(u"mailbox");
            if (mailboxIt != obj.end() && !mailboxIt->second.IsNull()) {
                mailFrom.m_mailbox = Entity::MailBox::FromConfiguration(mailboxIt->second);
            }

            mailFrom.m_nullReversePath = GetBoolOrDefault(
                obj,
                u"nullReversePath",
                false
            );

            mailFrom.m_displayName = GetStringOrDefault(
                obj,
                u"displayName",
                ""
            );

            mailFrom.m_mailSize = GetSizeTOrDefault(
                obj,
                u"mailSize",
                0
            );

            mailFrom.m_body = static_cast<MailBody>(
                GetInt64OrDefault(
                    obj,
                    u"body",
                    static_cast<int64_t>(MailBody::Unspecified)
                )
            );

            mailFrom.m_sourceType = static_cast<MailSourceType>(
                GetInt64OrDefault(
                    obj,
                    u"sourceType",
                    static_cast<int64_t>(MailSourceType::Unknown)
                )
            );

            /*
             * 优先解析 address 字段；
             * 如果 address 不存在，再用 addressString 解析。
             */
            auto addressIt = obj.find(u"address");

            if (addressIt != obj.end() && !addressIt->second.IsNull()) {
                mailFrom.m_address = Address::EmailAddress::FromConfiguration(addressIt->second);
                mailFrom.m_addressString = mailFrom.m_address.ToString();
            } else {
                mailFrom.m_addressString = GetStringOrDefault(
                    obj,
                    u"addressString",
                    ""
                );

                if (!mailFrom.m_addressString.empty()) {
                    Address::EmailAddress parsedAddress;

                    if (!Address::EmailAddress::Parse(mailFrom.m_addressString, parsedAddress)) {
                        throw std::runtime_error(
                            "Invalid MailFrom addressString: " + mailFrom.m_addressString
                        );
                    }

                    mailFrom.m_address = parsedAddress;
                }
            }
            return mailFrom;
        }

        bool MailFrom::ParseConfiguration(
            const LikesProgram::Configuration& cfg,
            MailFrom& out
        ) noexcept {
            try {
                out = MailFrom::FromConfiguration(cfg);
                return true;
            } catch (...) {
                out.Reset();
                return false;
            }
        }

        void MailFrom::Reset() {
            m_address.Reset();
            m_addressString.clear();
            m_displayName.clear();
            m_mailSize = 0;
            m_body = MailBody::Unspecified;
        }
    }
}