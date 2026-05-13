package com.example.novamailservice.dto.Result;

public final class ErrorCode {
    private ErrorCode() {} // 防止实例化

    // ---------------- 通用 ----------------
    public static final int SUCCESS = 200;               // 成功
    public static final int INTERNAL_ERROR = 500;       // 系统内部错误
    public static final int PARAM_INVALID = 400;        // 参数无效
    public static final int DATABASE_ERROR = 503;       // 数据库访问异常
    public static final int NOT_FOUND = 404;            // 资源不存在
    public static final int UNAUTHORIZED = 401;         // 未授权
    public static final int FORBIDDEN = 403;            // 禁止访问
    public static final int CONFLICT = 409;             // 冲突（如重复创建）

    // ---------------- 缓存相关 ----------------
    public static final int CACHE_MISS = 1001;           // 缓存未命中
    public static final int CACHE_ERROR = 1002;          // 缓存访问异常

    // ---------------- 用户相关 ----------------
    public static final int USER_NOT_FOUND = 2001;       // 用户不存在
    public static final int USER_EXISTS = 2002;          // 用户已存在
    public static final int USER_INVALID = 2003;         // 用户信息不合法
    public static final int USER_PASSWORD_ERROR = 2004;  // 密码错误
    public static final int USER_LOCKED = 2005;          // 用户被锁定
    public static final int USER_DISABLED = 2006;        // 用户被禁用
    public static final int USER_REGISTER_FAIL = 2007;   // 注册失败
    public static final int USER_PHONE_MISMATCH = 2008;  // 账号与验证码手机号不一致

    // ---------------- 认证相关 ----------------
    public static final int TOKEN_EXPIRED = 4011;
    public static final int TOKEN_ROBBED = 4012;
    public static final int TOKEN_INVALID = 4013;

    // ---------------- 邮箱相关 ----------------
    public static final int MAILBOX_NOT_FOUND = 3001;       // 邮箱不存在
    public static final int MAILBOX_EXISTS = 3002;          // 邮箱已存在
    public static final int MAILBOX_INVALID = 3003;         // 邮箱名称不合法
    public static final int MAILBOX_CREATE_FAIL = 3004;     // 邮箱创建失败
    public static final int MAILBOX_UNIQUE_FAIL = 3005;     // 无法生成唯一邮箱名

    // ---------------- 域名相关 ----------------
    public static final int DOMAIN_NOT_FOUND = 4001;    // 域名不存在
    public static final int DOMAIN_INVALID = 4002;      // 域名不合法

    // ---------------- 验证码 ----------------
    public static final int VC_IMAGE_GENERATE_FAIL = 5001;     // 图片验证码生成失败
    public static final int VC_IMAGE_INVALID = 5002;           // 图片验证码错误
    public static final int VC_IMAGE_EXPIRED = 5003;           // 图片验证码过期
    public static final int VC_SMS_SEND_FAIL = 5101;           // 短信验证码发送失败
    public static final int VC_SMS_INVALID_PHONE = 5102;       // 手机号格式错误
    public static final int VC_SMS_RESTRICT_IP = 5103;         // IP发送限制
    public static final int VC_SMS_RESTRICT_PHONE = 5104;      // 手机发送限制
    public static final int VC_SMS_CODE_ERROR = 5105;          // 验证码错误
    public static final int VC_SMS_CODE_EXPIRED = 5106;        // 验证码过期
    public static final int VC_SMS_PLATFORM_ERROR = 5107;      // 第三方短信服务异常

    // ---------------- 微服务 ----------------
    public static final int HUB_ERROR = 6001;               // 服务中心错误
}
