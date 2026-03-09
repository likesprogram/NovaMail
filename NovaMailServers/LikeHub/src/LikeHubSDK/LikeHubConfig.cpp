#include <LikeHub/LikeHubConfig.hpp>
#include <LikesProgram/String.hpp>
#include <system_error>
#include <cerrno>
#include <fstream>
#include <sstream>
#include <iostream>

namespace LikeHub {
    std::function <void(LikesProgram::Configuration&)> LikeHubConfig::m_applyDefaultsIfMissing = nullptr;

    // 辅助函数：将 errno 转换为字符串
    static std::string ErrnoMessage(int e) {
        return std::error_code(e, std::generic_category()).message();
    }

    // 辅助函数：构造包含路径和错误信息的异常
    static std::runtime_error FsError(const char* op, const std::filesystem::path& p, const std::error_code& ec, int savedErrno = 0) {
        std::string msg;
        msg.reserve(256);

        msg += op;
        msg += ": path='";
        msg += p.string();
        msg += "'";

        if (ec) {
            msg += ", fs_ec=";
            msg += std::to_string(ec.value());
            msg += " (";
            msg += ec.message();
            msg += ")";
        }

        if (savedErrno != 0) {
            msg += ", errno=";
            msg += std::to_string(savedErrno);
            msg += " (";
            msg += ErrnoMessage(savedErrno);
            msg += ")";
        }

        return std::runtime_error(msg);
    }

    LikeHubConfig& LikeHubConfig::Instance(std::function <void(LikesProgram::Configuration&)> applyDefaultsIfMissing) {
        static LikeHubConfig inst(applyDefaultsIfMissing);
        return inst;
    }

    bool LikeHubConfig::LoadConfig(std::string_view dir, std::string_view file) {
        const std::filesystem::path p = std::filesystem::path(dir) / std::string(file);
        // 文件不存在：保持默认配置
        if (!std::filesystem::exists(p)) return false;

        LikesProgram::String jsonText = ReadAllText(p);
        if (jsonText.Empty()) return false; // 文件为空
        // 反序列化
        m_config.Load(jsonText);
    
        // 补全缺失字段
        if (m_applyDefaultsIfMissing) m_applyDefaultsIfMissing(m_config);

        return true;
    }

    void LikeHubConfig::SaveConfig(std::string_view dir, std::string_view file) {
        const std::filesystem::path p = std::filesystem::path(dir) / std::string(file);
        if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    
        LikesProgram::String jsonText = m_config.Dump(4);

        WriteAllText(p, jsonText);
    }

    const LikesProgram::Configuration& LikeHubConfig::Raw() const noexcept { return m_config; }
    
    LikesProgram::String LikeHubConfig::ReadAllText(const std::filesystem::path& p) {
        // 先做 filesystem 级检查，给出“语义更明确”的错误
        std::error_code ec;

        if (!std::filesystem::exists(p, ec)) {
            // exists 本身失败或确实不存在
            throw FsError("ReadAllText: exists failed or not exists", p, ec);
        }
        if (!std::filesystem::is_regular_file(p, ec)) {
            throw FsError("ReadAllText: not a regular file", p, ec);
        }

        // 再打开文件，抓 errno
        errno = 0;
        std::wifstream ifs(p, std::ios::in);
        if (!ifs) {
            int e = errno; // 保存一下
            // 再补一个 fs 权限判断（可选）
            std::error_code ec2;
            auto perms = std::filesystem::status(p, ec2).permissions();
            (void)perms; // 你想打印 perms 也可以扩展
            throw FsError("ReadAllText: open failed", p, ec2, e);
        }

        std::wostringstream oss;
        oss << ifs.rdbuf();

        // 如果读过程中失败（例如权限/IO 错误），也应该报
        if (!ifs.good() && !ifs.eof()) {
            int e = errno;
            std::error_code ec3;
            throw FsError("ReadAllText: read failed", p, ec3, e);
        }

        return LikesProgram::String(oss.str());
    }
    
    void LikeHubConfig::WriteAllText(const std::filesystem::path& p, const LikesProgram::String& text) {
        // 先确保父目录存在（否则 open 会失败但信息不直观）
        std::error_code ec;
        auto parent = p.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent, ec)) {
            throw FsError("WriteAllText: parent directory not exists", parent, ec);
        }

        errno = 0;
        std::wofstream ofs(p, std::ios::out | std::ios::trunc);
        if (!ofs) {
            int e = errno;
            std::error_code ec2;
            throw FsError("WriteAllText: open failed", p, ec2, e);
        }

        ofs << text;

        // 强制把错误暴露出来：写/flush 失败要抛
        if (!ofs) {
            int e = errno;
            std::error_code ec3;
            throw FsError("WriteAllText: write failed", p, ec3, e);
        }

        ofs.flush();
        if (!ofs) {
            int e = errno;
            std::error_code ec4;
            throw FsError("WriteAllText: flush failed", p, ec4, e);
        }
    }

    void LikeHubConfig::EnsureObject(LikesProgram::Configuration& obj, const LikesProgram::String& key) {
        try {
            const LikesProgram::Configuration& cobj = obj;
            const auto& v = cobj[key];
            if (!v.IsObject()) throw std::runtime_error("not object");
        } catch (...) {
            LikesProgram::Configuration newObj;
            obj[key] = newObj; // 依赖你的 Configuration 支持赋值对象
        }
    }

    void LikeHubConfig::EnsureArray(LikesProgram::Configuration& obj, const LikesProgram::String& key, const LikesProgram::Configuration& defaultArray) {
        auto SetDefault = [&]() { obj[key] = defaultArray; };

        LikesProgram::Configuration* vptr = nullptr;
        try {
            vptr = &obj[key];
        } catch (...) {
            SetDefault();
            return;
        }

        if (!vptr->IsArray()) {
            SetDefault();
            return;
        }

        try {
            (void)(*vptr)[0];
        } catch (...) {
            SetDefault();
        }
    }

    LikeHubConfig::LikeHubConfig(std::function <void(LikesProgram::Configuration&)> applyDefaultsIfMissing) { 
        m_applyDefaultsIfMissing = applyDefaultsIfMissing;
        if (m_applyDefaultsIfMissing) m_applyDefaultsIfMissing(m_config);
    }
}
