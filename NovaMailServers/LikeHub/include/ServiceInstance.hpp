#pragma once
#include <string>                // std::string
#include <unordered_map>         // std::unordered_map
#include <cstdint>               // uint16_t/uint32_t/uint64_t
#include <optional>              // std::optional

namespace LikeHub {
    enum class LifecycleStatus : uint8_t {
        Registering = 0,   // 注册中（尚未完成握手）
        Active      = 1,   // 活跃（正常注册完成，正在提供服务）
        Deregistering = 2, // 正在下线（优雅关闭中）
        Removed     = 3    // 已删除（仅用于内部过渡态）
    };

    enum class HealthStatus : uint8_t {
        Unknown     = 0,   // 未知（刚注册/未探测）
        Healthy     = 1,   // 健康
        Unhealthy   = 2,   // 探测失败（仍保留）
        Suspected   = 3,   // 可疑（超时一次）
        Lost        = 4    // 失联（超时多次）
    };

    enum class TrafficStatus : uint8_t {
        Accepting = 0,   // 正常接流
        Draining  = 1,   // 排空中（不接新流量）
        Frozen    = 2,   // 冻结（运维手动摘流）
        Blocked   = 3    // 强制禁止（安全/熔断）
    };

    struct InstanceMetricsSnapshot {
        uint32_t cpuPct = 0; // CPU 使用率（0-100）
        uint32_t memPct = 0; // 内存使用率（0-100）

        uint64_t connActive = 0; // 当前活跃连接数
        uint64_t connectionsLimit = 0; // 连接数上限（0 表示未设置/无限制）
        uint64_t connectionsAvailable = 0; // 可用连接数（上限 - 活跃，0 表示无限制）

        uint64_t inflight = 0; // 在途请求数：更灵敏的负载指标
        uint64_t qps = 0; // 请求/事务吞吐：用于负载评估

        uint64_t selfP95LatencyMs = 0;      // 实例自报，P95 延迟（ms）：可用于慢实例降权/剔除
        uint64_t observedP95LatencyMs = 0;  // 注册中心，P95 延迟（ms）：可用于慢实例降权/剔除

        uint64_t bytesInPerSec = 0; // 入站速率：网络压力指标
        uint64_t bytesOutPerSec = 0; // 出站速率：网络压力指标

        uint64_t timestampMs = 0; // 采样时间戳（ms），用于过期判定
    };

    class ServiceInstance final {
    public:
    private:
        // 身份与命名域
        std::string serviceName;     // 服务名：例如 "smtp" / "imap" / "auth"（服务发现的主键之一）
        std::string instanceId;      // 实例ID：全局唯一（建议 UUID 或 service+ip+port+uniq），用于幂等更新/下线
        std::string instanceName;    // 实例展示名：给控制台/日志使用（可选，不参与逻辑判定）

        // 网络与协议
        std::string ip;              // 实例IP：注册中心对外暴露的可达地址（支持 IPv4/IPv6 文本）
        uint16_t    port = 0;        // 实例端口：对外服务端口（0 表示未设置/非法）

        // 观测与负载
        InstanceMetricsSnapshot metrics; // 实例指标快照：由实例定期上报，供负载评估与健康判定使用

        // 状态与标签
        LifecycleStatus lifecycle = LifecycleStatus::Registering;  // 生命周期状态：注册中 -> 活跃 -> 下线中 -> 已删除（过渡态，最终会被清理掉）
        HealthStatus health = HealthStatus::Unknown;  // 健康状态：由探测结果更新（影响是否接流/剔除）
        TrafficStatus traffic = TrafficStatus::Accepting; // 流量状态：运维手动控制流量走向
    };
}
