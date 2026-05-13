#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <types/Types.hpp>
#include <LikesProgram/net/Buffer.hpp>

using namespace LikesProgram::Net;
namespace NovaMail {
    namespace Smtp {
        enum class ParamKind { Positional, KeyValue };
        enum class ValueRole { Item, DisplayName, Address, Mechanism, InitialResponse };
        
        struct ValuePart {
            ValueRole role{ValueRole::Item};
            std::string text;
        };

        struct CommandParam {
            ParamKind kind{};
            std::string key;
            std::vector<ValuePart> values; // 绝大多数 1~2 个
            std::string raw;
        };

        class Command {
        public:
            Command() = default;
            // 若有一整行，会自动消费一行
            Command(LikesProgram::Net::Buffer& in);

            Types::CommandVerb Verb() const noexcept { return m_verb; }
            std::span<const CommandParam> Params() const noexcept { return m_parameter; }
            std::string_view RawLine() const noexcept { return m_raw_line; }
            bool NoComplete() { return !m_complete; }
        private:
            Types::CommandVerb m_verb = Types::CommandVerb::Unknown; // 指令动词
            std::vector<CommandParam> m_parameter = std::vector<CommandParam>();
            std::string m_raw_line = std::string(); // 保存指令行，防止意外释放
            bool m_complete = false;                  // 指令行是否完整

            // 解析指令动词
            size_t ParseVerb() noexcept;
            // 解析指令参数
            void ParseParam() noexcept;
        };
    }
}
