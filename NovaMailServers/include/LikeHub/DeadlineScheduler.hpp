#pragma once
#include <chrono>
#include <functional>
#include <queue>
#include <string>
#include <mutex>
#include <memory>
#include <thread>
#include <condition_variable>

namespace LikeHub {
    class DeadlineScheduler {
    public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;
    using Callback = std::function<void()>;
    using TimerId = uint64_t;
    public:
        // 获取单例实例
        static DeadlineScheduler& Instance();
        
        ~DeadlineScheduler();

        // 禁止拷贝
        DeadlineScheduler(const DeadlineScheduler&) = delete;
        DeadlineScheduler& operator=(const DeadlineScheduler&) = delete;

        // 禁止移动
        DeadlineScheduler(DeadlineScheduler&&) = delete;
        DeadlineScheduler& operator=(DeadlineScheduler&&) = delete;

        // 指定时间执行（一次）
        TimerId Schedule(const TimePoint& timePoint, Callback callback);

        // 延迟执行（一次）
        TimerId ScheduleAfter(const Duration& delay, Callback callback);

        // 周期执行
        TimerId ScheduleEvery(const Duration& interval, Callback callback);

        // 取消任务
        void Cancel(TimerId timerId);
    private: // 私有数据结构
        struct TimerNode {
            TimerId id = 0;

            TimePoint expiry; // 执行时间点
            Callback callback; // 执行函数

            Duration interval{0}; // 周期执行间隔，0 = 一次性任务，> 0 = 周期任务
            std::atomic<bool> cancelled{false}; // 取消标志
        };

        struct Compare {
            bool operator()(const std::shared_ptr<TimerNode>& a, const std::shared_ptr<TimerNode>& b) const {
                return a->expiry > b->expiry; // 最早的优先
            }
        };
    private: // 私有函数成员
        DeadlineScheduler();

        void Run();
        TimerId AddTimer(const TimePoint& tp, Duration interval, Callback cb);
    private:
        // 任务队列（小根堆，按执行时间排序）
        std::priority_queue<std::shared_ptr<TimerNode>, std::vector<std::shared_ptr<TimerNode>>, Compare> m_pendingHeap;
        // 任务映射（TimerId → TimerNode，方便取消）
        std::unordered_map<TimerId, std::shared_ptr<TimerNode>> m_timers;

        std::thread m_thread; // 调度线程
        bool m_running = false; // 运行状态
        std::atomic<TimerId> m_nextId = 1; // 任务ID生成器

        // 线程安全
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
    };
}