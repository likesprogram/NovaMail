#include <LikeHub/SystemMetrics.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace LikeHub {

    SystemMetrics& SystemMetrics::Instance() {
        static SystemMetrics instance;
        return instance;
    }

    MetricsSnapshot SystemMetrics::Sample() {
        std::lock_guard<std::mutex> lock(m_mutex);

        MetricsSnapshot snapshot;

        CpuTimes currentCpuTimes;
        if (ReadCpuTimes(currentCpuTimes)) {
            if (m_hasPrevCpuTimes) {
                snapshot.cpuPct = CalculateCpuUsagePct(m_prevCpuTimes, currentCpuTimes);
            } else {
                snapshot.cpuPct = 0.0;
                m_hasPrevCpuTimes = true;
            }

            m_prevCpuTimes = currentCpuTimes;
        }

        snapshot.memPct = ReadMemoryUsagePct();

        // 简单压力指标：可以后续扩展成负载、队列长度、连接数、Pending请求数等综合指标
        snapshot.stressIndicators = std::clamp(
            snapshot.cpuPct * 0.6 + snapshot.memPct * 0.4,
            0.0,
            100.0
        );

        return snapshot;
    }

    bool SystemMetrics::ReadCpuTimes(CpuTimes& out) {
#ifdef __linux__
        std::ifstream file("/proc/stat");
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        std::getline(file, line);

        std::istringstream iss(line);

        std::string cpuLabel;
        iss >> cpuLabel;

        if (cpuLabel != "cpu") {
            return false;
        }

        iss >> out.user
            >> out.nice
            >> out.system
            >> out.idle
            >> out.iowait
            >> out.irq
            >> out.softirq
            >> out.steal;

        return true;
#else
        return false;
#endif
    }

    double SystemMetrics::CalculateCpuUsagePct(const CpuTimes& prev, const CpuTimes& curr) {
        const uint64_t prevIdle = prev.IdleTime();
        const uint64_t currIdle = curr.IdleTime();

        const uint64_t prevTotal = prev.TotalTime();
        const uint64_t currTotal = curr.TotalTime();

        if (currTotal <= prevTotal) {
            return 0.0;
        }

        const uint64_t totalDelta = currTotal - prevTotal;
        const uint64_t idleDelta = currIdle > prevIdle ? currIdle - prevIdle : 0;

        if (totalDelta == 0) {
            return 0.0;
        }

        const double usage = static_cast<double>(totalDelta - idleDelta) * 100.0
                           / static_cast<double>(totalDelta);

        return std::clamp(usage, 0.0, 100.0);
    }

    double SystemMetrics::ReadMemoryUsagePct() {
#ifdef __linux__
        std::ifstream file("/proc/meminfo");
        if (!file.is_open()) {
            return 0.0;
        }

        std::string key;
        uint64_t value = 0;
        std::string unit;

        uint64_t memTotal = 0;
        uint64_t memAvailable = 0;

        while (file >> key >> value >> unit) {
            if (key == "MemTotal:") {
                memTotal = value;
            } else if (key == "MemAvailable:") {
                memAvailable = value;
            }

            if (memTotal != 0 && memAvailable != 0) {
                break;
            }
        }

        if (memTotal == 0 || memAvailable > memTotal) {
            return 0.0;
        }

        const uint64_t memUsed = memTotal - memAvailable;

        const double usage = static_cast<double>(memUsed) * 100.0
                           / static_cast<double>(memTotal);

        return std::clamp(usage, 0.0, 100.0);
#else
        return 0.0;
#endif
    }

}