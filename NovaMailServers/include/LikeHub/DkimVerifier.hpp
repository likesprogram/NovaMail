#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace LikesProgram::Net {
    class Buffer;
}

namespace LikeHub {
    enum class DkimVerifyStatus {
        Pass,
        Fail,
        None,
        TempFail,
        PermFail
    };

    struct DkimVerifyResult {
        DkimVerifyStatus status = DkimVerifyStatus::None;

        std::string domain;
        std::string selector;
        std::string identity;
        std::string algorithm;
        std::string canonicalization;
        std::string reason;
    };

    enum class DkimCanonicalization {
        Simple,
        Relaxed
    };

    struct DkimSignOptions {
        std::string domain;             // d=
        std::string selector;           // s=
        std::string privateKeyPem;      // PEM 私钥内容
        std::string identity;           // i=，可选，例如 user@example.com

        DkimCanonicalization headerCanon = DkimCanonicalization::Relaxed;
        DkimCanonicalization bodyCanon = DkimCanonicalization::Relaxed;

        std::vector<std::string> headersToSign = {
            "From",
            "To",
            "Subject",
            "Date",
            "Message-ID",
            "MIME-Version",
            "Content-Type",
            "Content-Transfer-Encoding"
        };

        bool includeBodyLength = false; // 是否生成 l=
    };

    class DkimVerifier {
    public:
        static DkimVerifyResult Verify(const LikesProgram::Net::Buffer& rawMail);
        static DkimVerifyResult Verify(std::string_view rawMail);

        // 返回完整 DKIM-Signature 头，不包含最终空行。
        // 使用方式：signedMail = dkimHeader + "\r\n" + rawMail;
        static std::string Sign(std::string_view rawMail, const DkimSignOptions& options);
    };

}