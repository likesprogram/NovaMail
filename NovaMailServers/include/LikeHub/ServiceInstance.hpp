#pragma once
#include <string>                // std::string
#include <unordered_map>         // std::unordered_map
#include <cstdint>               // uint16_t/uint32_t/uint64_t
#include <optional>              // std::optional
#include <limits>
#include <algorithm>
#include <atomic>
#include <LikesProgram/net/Address.hpp>

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

    struct InstanceMetricsSnapshot {
        uint32_t cpuPct = 0; // CPU 使用率（0-100）
        uint32_t memPct = 0; // 内存使用率（0-100）
        uint64_t stressIndicators = 0; // 当前活跃连接数
        uint64_t delayMs  = 0; // 延迟
    };

    struct ServiceLoadSnapshot {
        InstanceMetricsSnapshot metrics;
        double cpu = 0.0;
        double mem = 0.0;
        double stress = 0.0;
        double lat = 0.0;
        double load = 0.0;
        double effectiveLoad = 0.0;
    };

    struct ServiceSnapshot {
        uint64_t serviceId = 0;
        std::string serviceName;
        std::string serviceAddress;
        LifecycleStatus lifecycle = LifecycleStatus::Registering;
        HealthStatus health = HealthStatus::Unknown;
        ServiceLoadSnapshot load;
    };

    struct ServiceInstance {
        // 身份与命名域
        std::string serviceName;     // 服务名：例如 "smtp" / "imap" / "auth"（服务发现的主键之一）
        uint64_t instanceId;         // 实例ID：全局唯一（建议 UUID 或 service+ip+port+uniq），用于幂等更新/下线

        // 网络与协议
        LikesProgram::Net::Address serviceAddress; // 实例IP：注册中心对外暴露的可达地址（支持 IPv4/IPv6 文本）

        // 状态与标签
        std::atomic<LifecycleStatus> lifecycle{LifecycleStatus::Registering}; // 生命周期状态：注册中 -> 活跃 -> 下线中 -> 已删除（过渡态，最终会被清理掉）
        std::atomic<HealthStatus> health{HealthStatus::Unknown}; // 健康状态：由探测结果更新（影响是否接流/剔除）
    };
}
