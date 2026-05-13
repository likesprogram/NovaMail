#pragma once
#include <LikesProgram/Configuration.hpp>
#include <string>
#include <string_view>
#include <filesystem>
#include <functional>

namespace LikeHub {
    class LikeHubConfig {
    public:
        static LikeHubConfig& Instance(std::function <void(LikesProgram::Configuration&)> applyDefaultsIfMissing = nullptr);

        // 禁止拷贝
        LikeHubConfig(const LikeHubConfig&) = delete;
        LikeHubConfig& operator=(const LikeHubConfig&) = delete;

        // 禁止移动
        LikeHubConfig(LikeHubConfig&&) = delete;
        LikeHubConfig& operator=(LikeHubConfig&&) = delete;

        // 读取配置
        bool LoadConfig(std::string_view dir = "./", std::string_view file = "Configuration.json");

        // 保存配置
        void SaveConfig(std::string_view dir = "./", std::string_view file = "Configuration.json");

        const LikesProgram::Configuration& Raw() const noexcept;

        // IO helpers
        static LikesProgram::String ReadAllText(const std::filesystem::path& p);
        // IO helpers
        static void WriteAllText(const std::filesystem::path& p, const LikesProgram::String& text);

        // 确保键值存在，如果不存在则创建
        template<class T>
        static void EnsureKey(LikesProgram::Configuration& obj, const LikesProgram::String& key, const T& defValue) {
            try {
                const LikesProgram::Configuration& cobj = obj;
                (void)cobj[key];
            } catch (...) {
                obj[key] = defValue;
            }
        }

        // 确保对象存在，如果不存在则创建
        static void EnsureObject(LikesProgram::Configuration& obj, const LikesProgram::String& key);

        // 确保数组存在，如果不存在则创建
        static void EnsureArray(LikesProgram::Configuration& obj, const LikesProgram::String& key, const LikesProgram::Configuration& defaultArray);
    private:
        // 创建默认配置
        LikeHubConfig(std::function <void(LikesProgram::Configuration&)> applyDefaultsIfMissing);
    private:
        LikesProgram::Configuration m_config;
        static std::function <void(LikesProgram::Configuration&)> m_applyDefaultsIfMissing;
    };
}
