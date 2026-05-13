#pragma once

namespace NovaMail {
    namespace Smtp::Types {
        enum class ReplyCode : int {
            ServiceReady = 220,   // 服务就绪
            ClosingChannel = 221, // 服务即将关闭
            AuthSucceeded = 235,  // 认证成功
            Ok = 250,             // 请求成功
            AuthContinue = 334,   // AUTH 继续挑战
            StartMailInput = 354, // 开始邮件输入
            LocalError = 451,     // 临时错误
            InsufficientStorage = 452, // 存储队列已满
            AuthServiceUnavailable = 454, // 认证服务不可用
            SyntaxError = 500,    // 命令无法识别
            ParamError = 501,     // 参数错误
            NotImplemented = 502, // 命令未实现
            BadSequence = 503,    // 命令顺序错误
            AuthenticationRequired = 530, // 未认证
            AuthFailed = 535,     // 认证失败
            EncryptionRequiredForAuth = 538, // 需要加密 才能 进行认证
            MailboxUnavailable = 550, // 邮箱不存在
            MessageTooLarge = 552, // 邮件过大
            MailboxNameNotAllowed = 553, // 邮箱不属于认证的用户
            RelayDenied = 554,          // 中继访问被拒绝
            ParamUnsupported = 555// 参数不支持
        };
    }
}
