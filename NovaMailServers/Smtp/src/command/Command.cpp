#include <command/Command.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <core/Utils.hpp>
#include <iostream>

namespace NovaMail {
    namespace Smtp {
        // 参数分割
        static std::vector<std::string> SplitTokens(const std::string& line) {
            std::vector<std::string> tokens;
            size_t len = line.size();
            size_t i = 0;
        
            while (i < len) {
                // 跳过空格
                while (i < len && Core::Utils::IsBlank(line[i])) ++i;
                if (i >= len) break;
            
                size_t tokenStart = i;
                bool inQuotes = false;
                bool inAngle = false;
                bool sawSeparator = false; // : 或 = 
            
                while (i < len) {
                    char c = line[i];
                
                    if (c == '"' && !inAngle) {
                        inQuotes = !inQuotes;
                        ++i;
                        continue;
                    }
                    if (c == '<' && !inQuotes) {
                        inAngle = true;
                        ++i;
                        continue;
                    }
                    if (c == '>' && inAngle) {
                        inAngle = false;
                        ++i;
                        continue;
                    }
                    if ((c == ':' || c == '=') && !inQuotes && !inAngle && !sawSeparator) {
                        sawSeparator = true;
                        ++i;
                        continue;
                    }
                
                    if (Core::Utils::IsBlank(c) && !inQuotes && !inAngle) {
                        if (sawSeparator) {
                            size_t j = i;
                            while (j < len && Core::Utils::IsBlank(line[j])) ++j;
                            if (j < len) {
                                // 如果下一个 token 可能是 key: 或 key=
                                if (isalpha(line[j])) {
                                    size_t k = j;
                                    while (k < len && isalpha(line[k])) ++k;
                                    if (k < len && (line[k] == ':' || line[k] == '=')) break; // token 结束
                                }
                            }
                        } else break; // 普通 token 遇到空格就结束
                    }
                    ++i;
                }
                tokens.emplace_back(line.substr(tokenStart, i - tokenStart));
            }
            return tokens;
        }

        // 解析参数值
        static void ParseKeyValue(const std::string& value, CommandParam& param) noexcept {
            size_t i = 0;
            const size_t n = value.size();

            // 跳过空白字符
            auto skipBlankLocal = [&](size_t& p) { while (p < n && Core::Utils::IsBlank(value[p])) ++p; };
        
            skipBlankLocal(i);
        
            // 解析显示名："Display Name"
            if (i < n && value[i] == '"') {
                ++i;
                size_t start = i;
            
                while (i < n) {
                    if (value[i] == '"') break;
                    // TODO: 处理转义字符
                    ++i;
                }
            
                if (i <= n) {
                    param.values.push_back({ValueRole::DisplayName, value.substr(start, i - start)});
                    if (i < n && value[i] == '"') ++i;
                }
            }
        
            skipBlankLocal(i); // 跳过显示名后空格
        
            // 解析地址：<address>
            if (i < n && value[i] == '<') {
                ++i;
                size_t start = i;
            
                while (i < n && value[i] != '>') ++i;
            
                param.values.push_back({
                    ValueRole::Address,
                    value.substr(start, i - start)
                });
            
                if (i < n && value[i] == '>') ++i;
            }
        
            skipBlankLocal(i); // 跳过地址后空格
        
            // 剩余内容, 既不是 显示名也不是地址，作为 Item
            if (i < n) param.values.push_back({ValueRole::Item, value.substr(i)});
        }

        static size_t FindTopLevelSeparator(const std::string& token) noexcept {
            bool inQuotes = false;
            bool inAngle = false;

            for (size_t i = 0; i < token.size(); ++i) {
                const char c = token[i];
                if (c == '"' && !inAngle) {
                    inQuotes = !inQuotes;
                    continue;
                }
                if (c == '<' && !inQuotes) {
                    inAngle = true;
                    continue;
                }
                if (c == '>' && inAngle) {
                    inAngle = false;
                    continue;
                }
            
                if (!inQuotes && !inAngle && (c == ':' || c == '=')) return i;
            }
        
            return std::string::npos;
        }

        // 从左侧开始，找到第一个非空字符位置
        static size_t TrimLeftPos(const std::string& str, size_t pos = 0) noexcept {
            while (pos < str.size() && Core::Utils::IsBlank(str[pos])) ++pos;
            return pos;
        }

        // 从右侧开始，找到第一个非空字符位置
        static size_t TrimRightPos(const std::string& str, size_t pos) noexcept {
            while (pos > 0 && Core::Utils::IsBlank(str[pos - 1])) --pos;
            return pos;
        }

        // 去除首尾空白字符
        static std::string TrimCopy(const std::string& str) {
            size_t begin = TrimLeftPos(str, 0);
            size_t end = TrimRightPos(str, str.size());
            return (begin < end) ? str.substr(begin, end - begin) : std::string();
        }

        // 解析单个 Token
        static CommandParam ParseSingleToken(Types::CommandVerb verb, const std::string& token, size_t paramIndex) noexcept {
            CommandParam param;
            param.raw = token;

            const size_t sep = FindTopLevelSeparator(token);

            // 没有分隔符 => Positional
            if (sep == std::string::npos) {
                param.kind = ParamKind::Positional;
                param.key.clear();
            
                ValueRole role = ValueRole::Item;
                if (verb == Types::CommandVerb::AUTH && paramIndex == 0) role = ValueRole::Mechanism;
                else if (verb == Types::CommandVerb::AUTH && paramIndex == 1) role = ValueRole::InitialResponse;
            
                param.values.push_back({ role, token });
                return param;
            }

            // KeyValue
            param.kind = ParamKind::KeyValue;
        
            const size_t keyEnd = TrimRightPos(token, sep);
            param.key = token.substr(0, keyEnd);
        
            std::string value = (sep + 1 < token.size()) ? token.substr(sep + 1) : std::string();
            value = TrimCopy(value);
        
            ParseKeyValue(value, param);
            return param;
        }

        Command::Command(LikesProgram::Net::Buffer& in) {
            // 在 buffer 中查找 CRLF
            const char* data = reinterpret_cast<const char*>(in.Peek());
            size_t len = in.ReadableBytes();

            // 查找行尾
            const char* crlf = nullptr;
            for (size_t i = 0; i + 1 < len; ++i) {
                if (data[i] == '\r' && data[i + 1] == '\n') {
                    crlf = data + i; break;
                }
            }

            // 没有完整一行，等待更多数据
            if (!crlf) return;

            // 得到完整的一行
            m_complete = true;

            // 构造一行（不包含 CRLF）
            size_t lineLen = static_cast<size_t>(crlf - data);
            m_raw_line = std::string(data, lineLen);

            // 消费这一行（+2 是 CRLF）
            in.Consume(lineLen + 2);

            // 解析 指令动词
            size_t verbLen = ParseVerb();

            // 指令动词解析完成，并且还有剩余字符，证明有参数
            if (verbLen > 0 && verbLen < lineLen) ParseParam(); // 解析指令参数
        }

        size_t Command::ParseVerb() noexcept {
            if (m_raw_line.empty()) return 0;
            if (m_raw_line.size() < 4) return 0; // SMTP动词没有小于 4 个字符的指令

            // 跳过前导空字符
            size_t beginning = 0;
            while (beginning < m_raw_line.size() && Core::Utils::IsBlank(m_raw_line[beginning])) ++beginning;
            if (beginning >= m_raw_line.size()) return 0; // 全是空白

            // 计算动词结尾
            size_t end = beginning;
            while (end < m_raw_line.size() && !Core::Utils::IsBlank(m_raw_line[end])) ++end;

            size_t verbLen = end - beginning;

            // 获取动词
            m_verb = Types::StringToVerb(m_raw_line.substr(beginning, verbLen));

            // 返回消耗长度
            return verbLen;
        }

        void Command::ParseParam() noexcept {
            m_parameter.clear();
            m_parameter.reserve(4);
        
            const char* s = m_raw_line.data();
            size_t len = m_raw_line.size();
            size_t i = ParseVerb();
        
            auto skipBlank = [&](size_t& p) { while (p < len && Core::Utils::IsBlank(s[p])) ++p; };
        
            skipBlank(i);

            // 分割参数 Tokenize parameters
            std::vector<std::string> tokens = SplitTokens(m_raw_line.substr(i));

            // 解析每个参数 Token
            for (size_t index = 0; index < tokens.size(); ++index) {
                const std::string& token = tokens[index];
                LogDebug(u"Token: {}", token);

                CommandParam param = ParseSingleToken(m_verb, token, index);

                // 日志输出参数解析结果
                LikesProgram::String valuesString;
                for (const auto& v : param.values) {
                    if (!valuesString.Empty()) valuesString += u", ";
                    valuesString.Append(u"{Role=");
                    valuesString.Append(LikesProgram::String(
                        v.role == ValueRole::Item ? "Item" :
                        v.role == ValueRole::DisplayName ? "DisplayName" :
                        v.role == ValueRole::Address ? "Address" :
                        v.role == ValueRole::Mechanism ? "Mechanism" :
                        v.role == ValueRole::InitialResponse ? "InitialResponse" :
                        "Unknown"
                    ));
                    valuesString.Append(u", Text=");
                    valuesString.Append(LikesProgram::String(v.text));
                    valuesString.Append(u"}");
                }
                LogDebug(u"Add parameter: key={} values=[{}]", param.key, valuesString);

                m_parameter.emplace_back(std::move(param));
            }
        }
    }
}