#include <Command.hpp>
#include <iostream>

namespace NovaMail {
    namespace Smtp {
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
            while (beginning < m_raw_line.size() && IsBlank(m_raw_line[beginning])) ++beginning;
            if (beginning >= m_raw_line.size()) return 0; // 全是空白

            // 计算动词结尾
            size_t end = beginning;
            while (end < m_raw_line.size() && !IsBlank(m_raw_line[end])) ++end;

            size_t verbLen = end - beginning;

            // 获取动词
            m_verb = StringToVerb(m_raw_line.substr(beginning, verbLen));

            // 返回消耗长度
            return verbLen;
        }

        void Command::ParseParam() noexcept {
        
            m_parameter.clear();
            m_parameter.reserve(4);
        
            const char* s = m_raw_line.data();
            size_t len = m_raw_line.size();
            size_t i = ParseVerb();
        
            auto skipBlank = [&](size_t& p) {
                while (p < len && IsBlank(s[p])) ++p;
            };
        
            skipBlank(i);
        
            while (i < len) {
            
                size_t tokenStart = i;
            
                while (i < len && !IsBlank(s[i])) ++i;
            
                std::string token(s + tokenStart, i - tokenStart);
            
                CommandParam param;
                param.raw = token;
            
                auto colon = token.find(':');
                auto equal = token.find('=');
            
                // KEY:VALUE
                if (colon != std::string_view::npos) {
                
                    param.kind = ParamKind::ColonPair;
                    param.key = token.substr(0, colon);
                
                    std::string value = token.substr(colon + 1);
                
                    const char* v = value.data();
                    size_t vl = value.size();
                    size_t p = 0;
                
                    // Display Name
                    if (p < vl && v[p] == '"') {
                    
                        size_t start = ++p;
                    
                        while (p < vl && v[p] != '"')
                            ++p;
                    
                        if (p <= vl) {
                        
                            param.values.push_back({
                                ValueRole::DisplayName,
                                std::string(v + start, p - start)
                            });
                        
                            ++p;
                        }
                    }
                
                    while (p < vl && IsBlank(v[p])) ++p;
                
                    // <address>
                    if (p < vl && v[p] == '<') {
                    
                        size_t start = ++p;
                    
                        while (p < vl && v[p] != '>')
                            ++p;
                    
                        param.values.push_back({
                            ValueRole::Address,
                            std::string(v + start, p - start)
                        });
                    
                    }
                    else {
                    
                        param.values.push_back({
                            ValueRole::Primary,
                            value
                        });
                    
                    }
                
                }
                // KEY=VALUE
                else if (equal != std::string_view::npos) {
                
                    param.kind = ParamKind::KeyValue;
                    param.key = token.substr(0, equal);
                
                    param.values.push_back({
                        ValueRole::Item,
                        token.substr(equal + 1)
                    });
                }
                // POSITIONAL
                else {
                
                    param.kind = ParamKind::Positional;
                    param.key = "";
                
                    ValueRole role = ValueRole::Primary;
                
                    if (m_verb == CommandVerb::AUTH && m_parameter.empty()) role = ValueRole::Mechanism;
                    else if (m_verb == CommandVerb::AUTH && m_parameter.size() == 1)
                        role = ValueRole::InitialResponse;
                
                    param.values.push_back({role, token});
                }
                m_parameter.emplace_back(std::move(param));
                skipBlank(i);
            }
        }

        CommandVerb Command::StringToVerb(std::string_view verb) noexcept {
            // 基础命令
            if (IEquals(verb, "EHLO")) return CommandVerb::EHLO;
            if (IEquals(verb, "HELO")) return CommandVerb::HELO;
        
            // 事务命令
            if (IEquals(verb, "MAIL")) return CommandVerb::MAIL;  // MAIL FROM:...
            if (IEquals(verb, "RCPT")) return CommandVerb::RCPT;  // RCPT TO:...
            if (IEquals(verb, "DATA")) return CommandVerb::DATA;  // DATA + <CRLF>.<CRLF>
        
            if (IEquals(verb, "RSET")) return CommandVerb::RSET;
            if (IEquals(verb, "NOOP")) return CommandVerb::NOOP;
            if (IEquals(verb, "QUIT")) return CommandVerb::QUIT;
        
            if (IEquals(verb, "VRFY")) return CommandVerb::VRFY;  // 常见：通常禁用/返回252或502
            if (IEquals(verb, "EXPN")) return CommandVerb::EXPN;  // 常见：通常禁用/返回502
            if (IEquals(verb, "HELP")) return CommandVerb::HELP;
        
            // RFC 5321 遗留邮件发送族
            if (IEquals(verb, "SEND")) return CommandVerb::SEND;
            if (IEquals(verb, "SOML")) return CommandVerb::SOML;
            if (IEquals(verb, "SAML")) return CommandVerb::SAML;
        
            // ESMTP 扩展
            if (IEquals(verb, "STARTTLS")) return CommandVerb::STARTTLS; // STARTTLS
            if (IEquals(verb, "AUTH")) return CommandVerb::AUTH;     // AUTH <mech> ...
            if (IEquals(verb, "BDAT")) return CommandVerb::BDAT;     // CHUNKING：BDAT <size> [LAST]
            
            // 其它扩展/队列触发
            if (IEquals(verb, "ETRN")) return CommandVerb::ETRN;     // 队列触发（on-demand mail relay）
            if (IEquals(verb, "ATRN")) return CommandVerb::ATRN;     // Authenticated TURN
        
            // 历史遗留
            if (IEquals(verb, "TURN")) return CommandVerb::TURN;     // 历史命令，通常直接502
        
            return CommandVerb::Unknown;
        }
    }
}