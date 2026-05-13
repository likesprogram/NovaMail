#pragma once
#include <cstdint>
#include <mutex>

namespace LikeHub {
    struct MetricsSnapshot {
        double cpuPct = 0.0;
        double memPct = 0.0;
        double stressIndicators = 0.0;
    };

    class SystemMetrics {
    public:
        static SystemMetrics& Instance();

        MetricsSnapshot Sample();

    private:
        SystemMetrics() = default;

        SystemMetrics(const SystemMetrics&) = delete;
        SystemMetrics& operator=(const SystemMetrics&) = delete;

    private:
        struct CpuTimes {
            uint64_t user = 0;
            uint64_t nice = 0;
            uint64_t system = 0;
            uint64_t idle = 0;
            uint64_t iowait = 0;
            uint64_t irq = 0;
            uint64_t softirq = 0;
            uint64_t steal = 0;

            uint64_t IdleTime() const {
                return idle + iowait;
            }

            uint64_t NonIdleTime() const {
                return user + nice + system + irq + softirq + steal;
            }

            uint64_t TotalTime() const {
                return IdleTime() + NonIdleTime();
            }
        };

    private:
        static bool ReadCpuTimes(CpuTimes& out);
        static double ReadMemoryUsagePct();
        static double CalculateCpuUsagePct(const CpuTimes& prev, const CpuTimes& curr);

    private:
        std::mutex m_mutex;
        bool m_hasPrevCpuTimes = false;
        CpuTimes m_prevCpuTimes;
    };

}