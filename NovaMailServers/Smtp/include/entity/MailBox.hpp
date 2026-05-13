#pragma once
#include <entity/User.hpp>
#include <cstdint>
#include <string>
#include <LikesProgram/Configuration.hpp>

namespace NovaMail {
    namespace Smtp::Entity {
        struct MailBox {
            uint64_t mailBoxId = 0;
            uint64_t userId = 0;
            uint64_t domainNameId = 0;

            std::string localPart;

            bool enabled = false;

            UserStatus userStatus = UserStatus::Normal;

            bool Valid() const noexcept {
                return mailBoxId != 0;
            }

            LikesProgram::Configuration ToConfiguration() const {
                LikesProgram::Configuration cfg;

                /*
                 * uint64_t 统一转字符串，避免超过 int64_t 上限后丢失精度或溢出。
                 */
                cfg[u"mailBoxId"] = LikesProgram::String(std::to_string(mailBoxId));
                cfg[u"userId"] = LikesProgram::String(std::to_string(userId));
                cfg[u"domainNameId"] = LikesProgram::String(std::to_string(domainNameId));

                cfg[u"localPart"] = LikesProgram::String(localPart);
                cfg[u"enabled"] = enabled;

                /*
                 * userStatus 这里按数值保存，避免依赖 enum 名称。
                 */
                cfg[u"userStatus"] = static_cast<int64_t>(userStatus);

                return cfg;
            }

            static MailBox FromConfiguration(const LikesProgram::Configuration& cfg) {
                if (!cfg.IsObject()) {
                    throw std::runtime_error("MailBox configuration must be object");
                }

                MailBox mailBox;

                const auto& obj = cfg.AsObject();

                mailBox.mailBoxId = GetUInt64(obj, u"mailBoxId", 0);
                mailBox.userId = GetUInt64(obj, u"userId", 0);
                mailBox.domainNameId = GetUInt64(obj, u"domainNameId", 0);

                mailBox.localPart = GetString(obj, u"localPart", "");

                mailBox.enabled = GetBool(obj, u"enabled", false);

                int64_t statusValue = GetInt64(obj, u"userStatus", static_cast<int64_t>(UserStatus::Normal));
                mailBox.userStatus = static_cast<UserStatus>(statusValue);

                return mailBox;
            }

        private:
            static bool HasKey(
                const LikesProgram::Configuration::ObjectMap& obj,
                const LikesProgram::String& key
            ) {
                return obj.find(key) != obj.end();
            }

            static std::string GetString(
                const LikesProgram::Configuration::ObjectMap& obj,
                const LikesProgram::String& key,
                const std::string& defaultValue
            ) {
                auto it = obj.find(key);

                if (it == obj.end() || it->second.IsNull()) {
                    return defaultValue;
                }

                return it->second
                    .AsString(LikesProgram::Configuration::CastPolicy::AutoConvert)
                    .ToStdString();
            }

            static bool GetBool(
                const LikesProgram::Configuration::ObjectMap& obj,
                const LikesProgram::String& key,
                bool defaultValue
            ) {
                auto it = obj.find(key);

                if (it == obj.end() || it->second.IsNull()) {
                    return defaultValue;
                }

                return it->second.AsBool(LikesProgram::Configuration::CastPolicy::AutoConvert);
            }

            static int64_t GetInt64(
                const LikesProgram::Configuration::ObjectMap& obj,
                const LikesProgram::String& key,
                int64_t defaultValue
            ) {
                auto it = obj.find(key);

                if (it == obj.end() || it->second.IsNull()) {
                    return defaultValue;
                }

                return it->second.AsInt64(LikesProgram::Configuration::CastPolicy::AutoConvert);
            }

            static uint64_t GetUInt64(
                const LikesProgram::Configuration::ObjectMap& obj,
                const LikesProgram::String& key,
                uint64_t defaultValue
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
                            "MailBox field '" + key.ToStdString() + "' is not valid uint64"
                        );
                    }

                    if (parsedLength != text.size()) {
                        throw std::runtime_error(
                            "MailBox field '" + key.ToStdString() + "' contains invalid characters"
                        );
                    }

                    return static_cast<uint64_t>(parsed);
                }

                if (value.IsInt() || value.IsInt64() || value.IsBool()) {
                    int64_t signedValue = value.AsInt64(LikesProgram::Configuration::CastPolicy::AutoConvert);

                    if (signedValue < 0) {
                        throw std::runtime_error(
                            "MailBox field '" + key.ToStdString() + "' cannot be negative"
                        );
                    }

                    return static_cast<uint64_t>(signedValue);
                }

                throw std::runtime_error(
                    "MailBox field '" + key.ToStdString() + "' cannot cast to uint64"
                );
            }
        };
    }
}
