#include <LikeHub/Snowflake.hpp>
#include <chrono>

namespace LikeHub {
    Snowflake& Snowflake::Instance() {
        static Snowflake instance;
        return instance;
    }

    void Snowflake::Init(uint16_t workerId) {
        Instance().m_workerId = workerId & WORKER_ID_MASK;
    }

    uint64_t Snowflake::NextId() {
        return Instance().next();
    }

    uint64_t Snowflake::next() {
        while (true) {
            uint64_t timestamp = currentTime();
            uint64_t last = m_lastTimestamp.load(std::memory_order_relaxed);

            // 时钟回拨
            if (timestamp < last) {
                timestamp = waitUntil(last);
            }

            uint16_t seq;
            if (timestamp == last) {
                seq = (m_sequence.fetch_add(1, std::memory_order_relaxed) + 1) & SEQUENCE_MASK;

                if (seq == 0) {
                    // 当前毫秒用完
                    timestamp = waitNextMillis(last);
                    continue;
                }
            } else {
                m_sequence.store(0, std::memory_order_relaxed);
                seq = 0;
            }

            if (m_lastTimestamp.compare_exchange_weak(
                    last, timestamp, std::memory_order_relaxed)) {

                return ((timestamp - EPOCH) << TIMESTAMP_SHIFT)
                    | ((uint64_t)m_workerId << WORKER_SHIFT)
                    | seq;
            }
        }
    }

    uint64_t Snowflake::currentTime() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
        ).count();
    }

    uint64_t Snowflake::waitNextMillis(uint64_t last) {
        uint64_t ts;
        do {
            ts = currentTime();
        } while (ts <= last);
        return ts;
    }

    uint64_t Snowflake::waitUntil(uint64_t last) {
        uint64_t ts;
        do {
            ts = currentTime();
        } while (ts < last);
        return ts;
    }

}