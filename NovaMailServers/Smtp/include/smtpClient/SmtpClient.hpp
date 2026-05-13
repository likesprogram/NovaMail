#pragma once
#include <LikesProgram/net/Client.hpp>
#include <LikesProgram/net/Address.hpp>
#include <LikesProgram/net/Connection.hpp>
#include <LikesProgram/net/Buffer.hpp>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace NovaMail::Smtp {
    enum class SmtpTlsPolicy {
        Disable,        // 不使用 TLS
        Opportunistic,  // 服务器支持 STARTTLS 就升级，不支持则继续明文
        Required        // 必须 STARTTLS，不支持或握手失败则投递失败
    };

    struct SmtpTlsOptions {
        SmtpTlsPolicy policy = SmtpTlsPolicy::Opportunistic;

        bool verifyPeer = true;
        std::string caFile;
        std::string caPath;
    };

    struct SslCtxDeleter {
        void operator()(SSL_CTX* ctx) const noexcept {
            if (ctx) {
                SSL_CTX_free(ctx);
            }
        }
    };

    using SslCtxPtr = std::unique_ptr<SSL_CTX, SslCtxDeleter>;

    struct SmtpResponse {
        int code = 0;
        std::vector<std::string> lines;
    };

    struct SmtpDeliveryResult {
        bool success = false;
        int lastCode = 0;
        std::string message;

        static SmtpDeliveryResult Ok() {
            return SmtpDeliveryResult{
                .success = true,
                .lastCode = 250,
                .message = "OK"
            };
        }

        static SmtpDeliveryResult Failed(int code, std::string msg) {
            return SmtpDeliveryResult{
                .success = false,
                .lastCode = code,
                .message = std::move(msg)
            };
        }
    };

    class SmtpClient {
    public:
        SmtpClient(
            const std::string& host,
            uint16_t port,
            std::string heloDomain, SmtpTlsOptions tlsOptions = {},
            std::chrono::milliseconds timeout = std::chrono::milliseconds(10000)
        );

        ~SmtpClient();

        SmtpClient(const SmtpClient&) = delete;
        SmtpClient& operator=(const SmtpClient&) = delete;

        /**
         * 直接投递邮件。
         *
         * @param mailFrom 发件人地址，不带尖括号，例如 user@example.com
         * @param rcptToList 收件人地址列表，不带尖括号
         * @param rawMail RFC 5322 原始邮件内容
         */
        SmtpDeliveryResult Deliver(
            const std::string& mailFrom,
            const std::vector<std::string>& rcptToList,
            const std::string& rawMail
        );

        struct SharedState {
            mutable std::mutex mutex;
            std::condition_variable cv;

            bool connected = false;
            bool closed = false;
            bool error = false;
            int errorCode = 0;

            bool tlsHandshakeDone = false;
            
            std::string receiveCache;
            int currentCode = 0;
            std::vector<std::string> currentLines;
            std::deque<SmtpResponse> responses;
        };

    private:
        bool WaitConnected();
        bool SendCommand(const std::string& command);
        bool SendDataBlock(const std::string& rawMail);
        bool WaitResponse(SmtpResponse& response, const std::vector<int>& expectedCodes);
        bool IsExpectedCode(int code, const std::vector<int>& expectedCodes) const;
        bool EnsureClientSslContext();
        bool TryStartTls(const SmtpResponse& ehloResponse, SmtpResponse& response);
        bool WaitTlsHandshakeDone();

        static bool EhloSupportsStartTls(const SmtpResponse& response);
        static std::string DotStuff(const std::string& rawMail);
        static std::string JoinLines(const std::vector<std::string>& lines);

    private:
        SslCtxPtr m_sslCtx;
        std::string m_host;
        uint16_t m_port = 25;
        std::string m_heloDomain;
        SmtpTlsOptions m_tlsOptions;
        std::chrono::milliseconds m_timeout;

        std::shared_ptr<SharedState> m_state;
        LikesProgram::Net::Client m_client;
    };

    // 注册中继发件事件
    void RegisterRelayPolicySendMailEvent();
}