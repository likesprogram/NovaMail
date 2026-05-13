#pragma once
#include <cstdint>
#include <atomic>

namespace LikeHub {
    class Snowflake {
    public:
        // 初始化（必须调用一次）
        static void Init(uint16_t workerId);

        // 生成ID
        static uint64_t NextId();

    private:
        Snowflake() = default;

        static Snowflake& Instance();

        uint64_t next();

    private:
        static constexpr uint64_t EPOCH = 1609459200000ULL;

        static constexpr int WORKER_BITS = 10;
        static constexpr int SEQUENCE_BITS = 12;

        static constexpr uint64_t WORKER_ID_MASK = (1ULL << WORKER_BITS) - 1;
        static constexpr uint64_t SEQUENCE_MASK = (1ULL << SEQUENCE_BITS) - 1;

        static constexpr int WORKER_SHIFT = SEQUENCE_BITS;
        static constexpr int TIMESTAMP_SHIFT = SEQUENCE_BITS + WORKER_BITS;

    private:
        std::atomic<uint64_t> m_lastTimestamp{0};
        std::atomic<uint16_t> m_sequence{0};

        uint16_t m_workerId{0};

    private:
        static uint64_t currentTime();
        static uint64_t waitNextMillis(uint64_t last);
        static uint64_t waitUntil(uint64_t last);
    };
}
