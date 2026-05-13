#include <smtpClient/SmtpClient.hpp>
#include <LikesProgram/net/transports/TcpTransport.hpp>
#include <LikeHub/LikeHubClient.hpp>
#include <LikeHub/EventListener.hpp>
#include <LikeHub/LikeHubConfig.hpp>
#include <LikesProgram/log/Logger.hpp>
#include <MailDispatcher.hpp>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <utility>
#include <LikeHub/mailParser/MailParser.hpp>

namespace NovaMail::Smtp {
    namespace {
        using LikesProgram::Net::Address;
        using LikesProgram::Net::Buffer;
        using LikesProgram::Net::Connection;
        using LikesProgram::Net::EventLoop;
        using LikesProgram::Net::Transport;
        using LikesProgram::Net::TcpTransport;

        static bool IsDigit3(const std::string& line) {
            return line.size() >= 3
                && std::isdigit(static_cast<unsigned char>(line[0]))
                && std::isdigit(static_cast<unsigned char>(line[1]))
                && std::isdigit(static_cast<unsigned char>(line[2]));
        }

        static int ParseCode(const std::string& line) {
            if (!IsDigit3(line)) {
                return 0;
            }

            return (line[0] - '0') * 100
                 + (line[1] - '0') * 10
                 + (line[2] - '0');
        }

        class SmtpClientConnection final : public Connection {
        public:
            SmtpClientConnection(
                SocketType fd,
                EventLoop* loop,
                std::unique_ptr<Transport> transport,
                std::shared_ptr<SmtpClient::SharedState> state
            ) : Connection(fd, loop, std::move(transport)),
                m_state(std::move(state)) {
                LogDebug(
                    u"SMTP client connection created: fd={} this={:P}",
                    static_cast<long long>(GetSocket()),
                    static_cast<const void*>(this)
                );
            }

            ~SmtpClientConnection() override {
                LogDebug(
                    u"SMTP client connection destroyed: fd={} this={:P}",
                    static_cast<long long>(GetSocket()),
                    static_cast<const void*>(this)
                );
            }

        protected:
            void OnConnected() override {
                auto state = m_state;
                if (!state) {
                    return;
                }

                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->connected = true;
                }

                state->cv.notify_all();

                LogDebug(
                    u"SMTP client connected: server={} client={}",
                    GetRemoteAddress().ToString(),
                    GetLocalAddress().ToString()
                );
            }
            
            void OnHandshakeDone() override {
                auto state = m_state;
                if (!state) {
                    return;
                }
            
                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->tlsHandshakeDone = true;
                }
            
                state->cv.notify_all();
            
                LogInfo(
                    u"SMTP client TLS handshake done: fd={}",
                    static_cast<long long>(GetSocket())
                );
            }
            void OnMessage(Buffer& in) override {
                auto state = m_state;
                if (!state) {
                    in.Consume(in.ReadableBytes());
                    return;
                }

                const size_t n = in.ReadableBytes();
                if (n == 0) {
                    return;
                }

                std::string chunk(reinterpret_cast<const char*>(in.Peek()), n);

                in.Consume(n);

                std::lock_guard<std::mutex> lock(state->mutex);

                state->receiveCache.append(chunk);

                for (;;) {
                    size_t pos = state->receiveCache.find('\n');
                    if (pos == std::string::npos) {
                        break;
                    }

                    std::string line = state->receiveCache.substr(0, pos);
                    state->receiveCache.erase(0, pos + 1);

                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }

                    if (line.empty()) {
                        continue;
                    }

                    LogDebug(
                        u"SMTP client received line: {}",
                        LikesProgram::String(line)
                    );

                    if (!IsDigit3(line)) {
                        state->currentLines.push_back(line);
                        continue;
                    }

                    const int code = ParseCode(line);

                    if (state->currentLines.empty()) {
                        state->currentCode = code;
                    }

                    state->currentLines.push_back(line);

                    /*
                     * SMTP 多行响应格式：
                     * 250-first line
                     * 250-second line
                     * 250 last line
                     *
                     * 第 4 个字符是 '-' 表示后面还有行；
                     * 第 4 个字符是空格或不存在，表示响应结束。
                     */
                    const bool multiline = line.size() >= 4 && line[3] == '-';

                    if (!multiline) {
                        SmtpResponse response;
                        response.code = state->currentCode != 0 ? state->currentCode : code;
                        response.lines = std::move(state->currentLines);

                        state->responses.push_back(std::move(response));

                        state->currentCode = 0;
                        state->currentLines.clear();

                        state->cv.notify_all();
                    }
                }
            }

            void OnClosed() override {
                auto state = m_state;
                if (!state) return;

                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->closed = true;
                }

                state->cv.notify_all();

                LogWarn(u"SMTP client closed: fd={}", static_cast<long long>(GetSocket()));
            }

            void OnError(int err) override {
                auto state = m_state;
                if (!state) return;

                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->error = true;
                    state->errorCode = err;
                }

                state->cv.notify_all();

                LogWarn(
                    u"SMTP client connection error: fd={} err={}",
                    static_cast<long long>(GetSocket()), err
                );
            }

        private:
            std::shared_ptr<SmtpClient::SharedState> m_state;
        };

        static LikesProgram::Net::ConnectionFactory MakeSmtpConnectionFactory(
            const std::shared_ptr<SmtpClient::SharedState>& state
        ) {
            return [state](
                SocketType fd,
                EventLoop* ownerLoop
            ) -> std::shared_ptr<Connection> {
                auto transport = std::make_unique<TcpTransport>(fd);

                return std::make_shared<SmtpClientConnection>(
                    fd,
                    ownerLoop,
                    std::move(transport),
                    state
                );
            };
        }

    }

    static LikesProgram::String ParseAction(const LikeHub::RpcMessage& request) {
        LikesProgram::String action = u"Unknown";
        try {
            const auto& metadata = request.MetaData();
            if (!metadata[u"action"].IsNull()) action = metadata[u"action"].AsString();
        } catch (const std::exception& e) {
            LogWarn(
                u"Hub service request mailId parse failed: error={} requestId={}",
                e.what(), request.Header().requestId
            );
        } catch (...) {
            LogWarn(
                u"Hub service request mailId parse failed: unknown error requestId={}", request.Header().requestId
            );
        }
        return action;
    }

    SmtpClient::SmtpClient(
        const std::string& host,
        uint16_t port,
        std::string heloDomain,
        SmtpTlsOptions tlsOptions,
        std::chrono::milliseconds timeout
    ) : m_host(host),
        m_port(port),
        m_heloDomain(std::move(heloDomain)),
        m_tlsOptions(std::move(tlsOptions)),
        m_timeout(timeout),
        m_state(std::make_shared<SharedState>()),
        m_client(
            LikesProgram::Net::Address(host, port),
            MakeSmtpConnectionFactory(m_state)
        ) {
    }

    SmtpClient::~SmtpClient() {
        try {
            m_client.Shutdown();
            m_client.WaitShutdown();
        } catch (...) {
        }
    }

    SmtpDeliveryResult SmtpClient::Deliver(
        const std::string& mailFrom,
        const std::vector<std::string>& rcptToList,
        const std::string& rawMail
    ) {
        if (mailFrom.empty()) {
            return SmtpDeliveryResult::Failed(0, "MAIL FROM is empty");
        }

        if (rcptToList.empty()) {
            return SmtpDeliveryResult::Failed(0, "RCPT TO list is empty");
        }

        if (rawMail.empty()) {
            return SmtpDeliveryResult::Failed(0, "Raw mail is empty");
        }

        LogInfo(
            u"SMTP outbound delivery started: host={} port={} rcptCount={} rawSize={}",
            LikesProgram::String(m_host),
            static_cast<int64_t>(m_port),
            static_cast<int64_t>(rcptToList.size()),
            static_cast<int64_t>(rawMail.size())
        );

        m_client.Start();

        if (!WaitConnected()) {
            return SmtpDeliveryResult::Failed(0, "Connect timeout or failed");
        }

        SmtpResponse response;

        if (!WaitResponse(response, {220})) {
            return SmtpDeliveryResult::Failed(
                response.code,
                "SMTP greeting failed: " + JoinLines(response.lines)
            );
        }

        if (!SendCommand("EHLO " + m_heloDomain)) {
            return SmtpDeliveryResult::Failed(0, "Send EHLO failed");
        }

        if (!WaitResponse(response, {250})) {
            return SmtpDeliveryResult::Failed(
                response.code,
                "EHLO failed: " + JoinLines(response.lines)
            );
        }

        if (!TryStartTls(response, response)) {
            return SmtpDeliveryResult::Failed(
                response.code,
                "STARTTLS failed: " + JoinLines(response.lines)
            );
        }

        if (!SendCommand("MAIL FROM:<" + mailFrom + ">")) {
            return SmtpDeliveryResult::Failed(0, "Send MAIL FROM failed");
        }

        if (!WaitResponse(response, {250})) {
            return SmtpDeliveryResult::Failed(
                response.code,
                "MAIL FROM failed: " + JoinLines(response.lines)
            );
        }

        size_t acceptedRcptCount = 0;

        for (const auto& rcptTo : rcptToList) {
            if (rcptTo.empty()) {
                continue;
            }

            if (!SendCommand("RCPT TO:<" + rcptTo + ">")) {
                return SmtpDeliveryResult::Failed(0, "Send RCPT TO failed");
            }

            /*
             * 250: OK
             * 251: User not local; will forward
             */
            if (WaitResponse(response, {250, 251})) {
                ++acceptedRcptCount;
                continue;
            }

            LogWarn(
                u"SMTP recipient rejected: host={} rcpt={} code={} message={}",
                LikesProgram::String(m_host),
                LikesProgram::String(rcptTo),
                response.code,
                LikesProgram::String(JoinLines(response.lines))
            );
        }

        if (acceptedRcptCount == 0) {
            return SmtpDeliveryResult::Failed(
                response.code,
                "No recipient accepted: " + JoinLines(response.lines)
            );
        }

        if (!SendCommand("DATA")) {
            return SmtpDeliveryResult::Failed(0, "Send DATA failed");
        }

        if (!WaitResponse(response, {354})) {
            return SmtpDeliveryResult::Failed(
                response.code,
                "DATA rejected: " + JoinLines(response.lines)
            );
        }

        if (!SendDataBlock(rawMail)) {
            return SmtpDeliveryResult::Failed(0, "Send DATA block failed");
        }

        if (!WaitResponse(response, {250})) {
            return SmtpDeliveryResult::Failed(
                response.code,
                "Message body rejected: " + JoinLines(response.lines)
            );
        }

        SendCommand("QUIT");
        WaitResponse(response, {221});

        LogInfo(
            u"SMTP outbound delivery completed: host={} acceptedRcptCount={}",
            LikesProgram::String(m_host),
            static_cast<int64_t>(acceptedRcptCount)
        );

        m_client.Shutdown();

        return SmtpDeliveryResult::Ok();
    }

    bool SmtpClient::WaitConnected() {
        std::unique_lock<std::mutex> lock(m_state->mutex);

        const bool ok = m_state->cv.wait_for(
            lock,
            m_timeout,
            [&]() {
                return m_state->connected || m_state->closed || m_state->error;
            }
        );

        if (!ok) {
            LogWarn(
                u"SMTP client connect timeout: host={} port={}",
                LikesProgram::String(m_host),
                static_cast<int64_t>(m_port)
            );

            return false;
        }

        if (m_state->error || m_state->closed) {
            LogWarn(
                u"SMTP client connect failed: host={} port={} error={} closed={}",
                LikesProgram::String(m_host),
                static_cast<int64_t>(m_port),
                m_state->errorCode,
                m_state->closed
            );

            return false;
        }

        return true;
    }

    bool SmtpClient::SendCommand(const std::string& command) {
        auto conn = m_client.GetConnection();
        if (!conn) {
            LogWarn(
                u"SMTP client send command failed: host={} command={} reason=ConnectionNull",
                LikesProgram::String(m_host),
                LikesProgram::String(command)
            );

            return false;
        }

        std::string data = command;
        data.append("\r\n");

        conn->Send(data.data(), data.size());

        LogDebug(
            u"SMTP client command sent: host={} command={}",
            LikesProgram::String(m_host),
            LikesProgram::String(command)
        );

        return true;
    }

    bool SmtpClient::SendDataBlock(const std::string& rawMail) {
        auto conn = m_client.GetConnection();
        if (!conn) {
            LogWarn(
                u"SMTP client send data failed: host={} reason=ConnectionNull",
                LikesProgram::String(m_host)
            );

            return false;
        }

        std::string data = DotStuff(rawMail);
        data.append(".\r\n");

        conn->Send(data.data(), data.size());

        LogDebug(
            u"SMTP client data block sent: host={} size={}",
            LikesProgram::String(m_host),
            static_cast<int64_t>(data.size())
        );

        return true;
    }

    bool SmtpClient::WaitResponse(
        SmtpResponse& response,
        const std::vector<int>& expectedCodes
    ) {
        response = SmtpResponse{};

        std::unique_lock<std::mutex> lock(m_state->mutex);

        const bool ok = m_state->cv.wait_for(
            lock,
            m_timeout,
            [&]() {
                return !m_state->responses.empty()
                    || m_state->closed
                    || m_state->error;
            }
        );

        if (!ok) {
            LogWarn(
                u"SMTP client wait response timeout: host={}",
                LikesProgram::String(m_host)
            );

            return false;
        }

        if (m_state->responses.empty()) {
            LogWarn(
                u"SMTP client wait response failed: host={} error={} closed={}",
                LikesProgram::String(m_host),
                m_state->errorCode,
                m_state->closed
            );

            return false;
        }

        response = std::move(m_state->responses.front());
        m_state->responses.pop_front();

        const bool expected = IsExpectedCode(response.code, expectedCodes);

        if (!expected) {
            LogWarn(
                u"SMTP client unexpected response: host={} code={} message={}",
                LikesProgram::String(m_host),
                response.code,
                LikesProgram::String(JoinLines(response.lines))
            );
        } else {
            LogDebug(
                u"SMTP client expected response: host={} code={} message={}",
                LikesProgram::String(m_host),
                response.code,
                LikesProgram::String(JoinLines(response.lines))
            );
        }

        return expected;
    }

    bool SmtpClient::IsExpectedCode(
        int code,
        const std::vector<int>& expectedCodes
    ) const {
        return std::find(
            expectedCodes.begin(),
            expectedCodes.end(),
            code
        ) != expectedCodes.end();
    }

    bool SmtpClient::EnsureClientSslContext() {
        if (m_sslCtx) {
            return true;
        }

        SSL_CTX* raw = SSL_CTX_new(TLS_client_method());
        if (!raw) {
            LogWarn(u"SMTP client create SSL_CTX failed");
            return false;
        }

        m_sslCtx.reset(raw);

        SSL_CTX_set_min_proto_version(m_sslCtx.get(), TLS1_2_VERSION);

        SSL_CTX_set_options(
            m_sslCtx.get(),
            SSL_OP_NO_SSLv2 |
            SSL_OP_NO_SSLv3 |
            SSL_OP_NO_COMPRESSION
        );

        SSL_CTX_set_mode(
            m_sslCtx.get(),
            SSL_MODE_ENABLE_PARTIAL_WRITE |
            SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER
        );

        if (m_tlsOptions.verifyPeer) {
            SSL_CTX_set_verify(m_sslCtx.get(), SSL_VERIFY_PEER, nullptr);

            bool caLoaded = false;

            if (!m_tlsOptions.caFile.empty() || !m_tlsOptions.caPath.empty()) {
                caLoaded = SSL_CTX_load_verify_locations(
                    m_sslCtx.get(),
                    m_tlsOptions.caFile.empty() ? nullptr : m_tlsOptions.caFile.c_str(),
                    m_tlsOptions.caPath.empty() ? nullptr : m_tlsOptions.caPath.c_str()
                ) == 1;
            } else {
                caLoaded = SSL_CTX_set_default_verify_paths(m_sslCtx.get()) == 1;
            }

            if (!caLoaded) {
                LogWarn(
                    u"SMTP client load CA failed: host={}",
                    LikesProgram::String(m_host)
                );

                m_sslCtx.reset();
                return false;
            }
        } else {
            SSL_CTX_set_verify(m_sslCtx.get(), SSL_VERIFY_NONE, nullptr);
        }

        return true;
    }

    bool SmtpClient::TryStartTls(
        const SmtpResponse& ehloResponse,
        SmtpResponse& response
    ) {
        if (m_tlsOptions.policy == SmtpTlsPolicy::Disable) {
            return true;
        }

        const bool supported = EhloSupportsStartTls(ehloResponse);

        if (!supported) {
            if (m_tlsOptions.policy == SmtpTlsPolicy::Required) {
                LogWarn(
                    u"SMTP STARTTLS required but not advertised: host={}",
                    LikesProgram::String(m_host)
                );
                return false;
            }

            LogInfo(
                u"SMTP STARTTLS not advertised, continue without TLS: host={}",
                LikesProgram::String(m_host)
            );
            return true;
        }

        if (!EnsureClientSslContext()) {
            return false;
        }

        if (!SendCommand("STARTTLS")) {
            return false;
        }

        if (!WaitResponse(response, {220})) {
            LogWarn(
                u"SMTP STARTTLS rejected: host={} code={} message={}",
                LikesProgram::String(m_host),
                response.code,
                LikesProgram::String(JoinLines(response.lines))
            );
            return false;
        }

        auto conn = m_client.GetConnection();
        if (!conn) {
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            m_state->tlsHandshakeDone = false;

            // STARTTLS 之后理论上不应该残留明文响应状态，清一下更稳。
            m_state->receiveCache.clear();
            m_state->currentCode = 0;
            m_state->currentLines.clear();
        }

        conn->StartTlsAfterWriteComplete(
            m_sslCtx.get(),
            LikesProgram::Net::TlsMode::Client
        );

        if (!WaitTlsHandshakeDone()) {
            return false;
        }

        /*
         * RFC 语义上 STARTTLS 完成后必须重新 EHLO。
         * 因为 TLS 前后的能力列表可能不同，尤其是 AUTH。
         */
        if (!SendCommand("EHLO " + m_heloDomain)) {
            return false;
        }

        if (!WaitResponse(response, {250})) {
            LogWarn(
                u"SMTP EHLO after STARTTLS failed: host={} code={} message={}",
                LikesProgram::String(m_host),
                response.code,
                LikesProgram::String(JoinLines(response.lines))
            );
            return false;
        }

        return true;
    }

    bool SmtpClient::WaitTlsHandshakeDone() {
        std::unique_lock<std::mutex> lock(m_state->mutex);

        const bool ok = m_state->cv.wait_for(
            lock,
            m_timeout,
            [&]() {
                return m_state->tlsHandshakeDone
                    || m_state->closed
                    || m_state->error;
            }
        );

        if (!ok) {
            LogWarn(
                u"SMTP client TLS handshake timeout: host={}",
                LikesProgram::String(m_host)
            );
            return false;
        }

        if (m_state->error || m_state->closed || !m_state->tlsHandshakeDone) {
            LogWarn(
                u"SMTP client TLS handshake failed: host={} error={} closed={}",
                LikesProgram::String(m_host),
                m_state->errorCode,
                m_state->closed
            );
            return false;
        }

        return true;
    }

    bool SmtpClient::EhloSupportsStartTls(const SmtpResponse& response) {
        for (const auto& rawLine : response.lines) {
            if (rawLine.size() < 4) {
                continue;
            }

            // 跳过 "250-" 或 "250 "
            std::string cap = rawLine.substr(4);

            while (!cap.empty() && std::isspace(static_cast<unsigned char>(cap.front()))) {
                cap.erase(cap.begin());
            }

            std::string upper;
            upper.reserve(cap.size());

            for (char ch : cap) {
                upper.push_back(static_cast<char>(
                    std::toupper(static_cast<unsigned char>(ch))
                ));
            }

            if (upper == "STARTTLS" || upper.rfind("STARTTLS ", 0) == 0) {
                return true;
            }
        }

        return false;
    }

    std::string SmtpClient::DotStuff(const std::string& rawMail) {
        std::string result;
        result.reserve(rawMail.size() + 16);

        std::istringstream input(rawMail);
        std::string line;

        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (!line.empty() && line.front() == '.') {
                result.push_back('.');
            }

            result.append(line);
            result.append("\r\n");
        }

        if (result.empty()) {
            result.append("\r\n");
        }

        return result;
    }

    std::string SmtpClient::JoinLines(const std::vector<std::string>& lines) {
        std::string result;

        for (size_t i = 0; i < lines.size(); ++i) {
            if (i != 0) {
                result.append(" | ");
            }

            result.append(lines[i]);
        }

        return result;
    }

    void RegisterRelayPolicySendMailEvent() {
        // 分类事件处理逻辑，单独提出来，避免 RegisterEvent 内部过度嵌套
        auto relayPolicySendMailHandler = [](const LikeHub::EventContext& ctx) -> bool {
            LikeHub::LikeHubClient& likeHub = LikeHub::LikeHubClient::Instance();
            LikesProgram::Configuration metadata;
            metadata[u"action"] = ParseAction(ctx.message);

            auto sendReply = [&](LikeHub::RpcMessage::Code code) {
                likeHub.SendMessage(LikeHub::RpcMessage::Encode(LikeHub::RpcMessage::Type::Response, ctx.message.Header().requestId, code, metadata, 0), 0);
            };

            Types::MailFrom mailFrom;
            std::vector<Types::RcptTo> rcptTos;
            try {
                if (ctx.message.Header().bodyLength <= 0 || ctx.message.BodyData().ReadableBytes() <= 0) {
                    LogError(u"RelayPolicySendMail failed: load mail data failed");
                    sendReply(LikeHub::RpcMessage::Code::NotFound);
                    return false;
                }
            
                const auto& meta = ctx.message.MetaData();
            
                // 读取 MailFrom
                if (!Types::MailFrom::ParseConfiguration(meta[u"mailFrom"], mailFrom)) {
                    LogError(u"RelayPolicySendMail failed: parse mailFrom failed");
                    sendReply(LikeHub::RpcMessage::Code::NotFound);
                    return false;
                }
            
                // 读取 RcptTos
                const auto& rcptTosCfg = meta[u"rcptTos"];
                if (!rcptTosCfg.IsArray()) {
                    LogError(u"RelayPolicySendMail failed: rcptTos is not array");
                    sendReply(LikeHub::RpcMessage::Code::NotFound);
                    return false;
                }
                for (const auto& cfg : rcptTosCfg) {
                    Types::RcptTo rcptTo;
                    if (!Types::RcptTo::ParseConfiguration(cfg, rcptTo)) {
                        LogError(u"RelayPolicySendMail failed: parse rcptTo failed");
                        sendReply(LikeHub::RpcMessage::Code::NotFound);
                        return false;
                    }
                    rcptTos.push_back(std::move(rcptTo));
                }
            
                if (rcptTos.empty()) {
                    LogError(u"RelayPolicySendMail failed: rcptTos is empty");
                    sendReply(LikeHub::RpcMessage::Code::NotFound);
                    return false;
                }
            
                // 先给服务端回复，避免超时
                sendReply(LikeHub::RpcMessage::Code::OK);
            }
            catch (const std::exception& e) {
                LogError(u"RelayPolicySendMail failed: exception={}", e.what());
                sendReply(LikeHub::RpcMessage::Code::NotFound);
                return false;
            }
            uint64_t mailId;
            std::string filePath;
            static std::string storageBucket = "mails";
            try {
                // 插入数据库
                Service::EmailService emailService(LikeHub::Storage::DatabasePool::Instance());
                if (!emailService.InsertMail(storageBucket, mailId, filePath, mailFrom, ctx.message.BodyData())) {
                    LogError(
                        u"SMTP message rejected: failed to store metadata, from=<{}> rcptCount={} size={}",
                        mailFrom.Address().ToString(),
                        rcptTos.size(),
                        std::to_string(ctx.message.BodyData().ReadableBytes())
                    );
                    return false;
                }

                // 拷贝数据
                auto buffer = std::make_shared<LikesProgram::Net::Buffer>();
                buffer->Append(ctx.message.BodyData().Peek(), ctx.message.BodyData().ReadableBytes());

                // 构造任务
                StoreTask task;
                task.mailId = mailId;
                task.bucket = storageBucket;
                task.filePath = filePath;
                task.mailFrom = mailFrom;
                task.rcpTos = rcptTos;
                task.data = buffer;
                task.retry = 0;

                // 投递任务
                if (!MailDispatcher::Instance().EnqueueStore(task)) {
                    LogError(u"SMTP message deferred: internal queue full, mailId={} from=<{}> rcptCount={} size={}",
                        mailFrom.Address().ToString(),
                        rcptTos.size(),
                        std::to_string(ctx.message.BodyData().ReadableBytes())
                    );
                    emailService.MarkFailed(mailId); // 写入失败
                    return false;
                }

                LogInfo(
                    u"SMTP message received: mailId={} from=<{}> rcptCount={} size={}",
                    mailFrom.Address().ToString(),
                    rcptTos.size(),
                    std::to_string(ctx.message.BodyData().ReadableBytes())
                );
                return true;
            } catch (const std::exception& e) {
                LogError(
                    u"SMTP message failed: exception={}, from=<{}> rcptCount={} size={}",
                    mailFrom.Address().ToString(),
                    rcptTos.size(),
                    std::to_string(ctx.message.BodyData().ReadableBytes())
                );
            } catch (...) {
                LogError(u"SMTP DATA processing unknown exception");
            }
            return false;
        };
        
        LikeHub::LikeHubClient& hubServer = LikeHub::LikeHubClient::Instance();
        hubServer.RegisterEvent(u"RelayPolicySendMail", relayPolicySendMailHandler, LikeHub::ListenType::Event);
    }
}