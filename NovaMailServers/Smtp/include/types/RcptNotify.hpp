#pragma once
#include <cstdint>

namespace NovaMail {
    namespace Smtp::Types {
        enum class RcptNotify : uint8_t {
            None    = 0,        // 未指定 NOTIFY 参数，使用服务器默认行为
            Success = 1 << 0,   // 当邮件成功投递到最终收件人邮箱时发送 DSN 成功通知
            Failure = 1 << 1,   // 当邮件投递失败（例如用户不存在、邮箱满、策略拒绝）时发送 DSN 失败通知
            Delay   = 1 << 2,   // 当邮件在队列中延迟超过服务器策略时间时发送延迟通知
            Never   = 1 << 3    // 永不发送 DSN 通知, 该值与 SUCCESS / FAILURE / DELAY 互斥
        };
                
        RcptNotify operator|(RcptNotify a, RcptNotify b);

        RcptNotify operator&(RcptNotify a, RcptNotify b);

        bool HasNotify(RcptNotify caps, RcptNotify test);

        bool IsValidNotify(RcptNotify flags);
    }
}