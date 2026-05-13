#pragma once

namespace NovaMail {
    namespace Smtp::Types {
        // SMTP 指令状态
        enum class State : int {
            Connected = 0,       // TCP 已建立，未发 220
            Greeted = 1,         // 已发 220
            Identified = 2,      // EHLO/HELO 完成（会话就绪/Idle）
            Authenticated = 3,   // 登录中
            MailStarted = 4,     // MAIL FROM 成功（事务进行中）
            RcptCollecting = 5,  // 至少一个 RCPT
            DataReceiving = 6,   // DATA 内容接收中
            BdatReceiving = 7,   // BDAT 内容接收中
        };
    }
}
