#include <LikeHub/DeadlineScheduler.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace LikeHub {
    DeadlineScheduler& DeadlineScheduler::Instance() {
        static DeadlineScheduler instance;
        return instance;
    }

    DeadlineScheduler::TimerId DeadlineScheduler::Schedule(const TimePoint& timePoint, Callback callback) {
        return AddTimer(timePoint, Duration::zero(), std::move(callback));
    }

    DeadlineScheduler::TimerId DeadlineScheduler::ScheduleAfter(const Duration& delay, Callback callback) {
        return AddTimer(Clock::now() + delay, Duration::zero(), std::move(callback));
    }

    DeadlineScheduler::TimerId DeadlineScheduler::ScheduleEvery(const Duration& interval, Callback callback) {
        if (interval <= Duration::zero()) return 0; // 防御性：非法 interval
        return AddTimer(Clock::now() + interval, interval, std::move(callback));
    }

    void DeadlineScheduler::Cancel(TimerId timerId) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            // 查找任务
            auto it = m_timers.find(timerId);
            if (it == m_timers.end()) return;

            // 懒删除（标记取消）
            it->second->cancelled.store(true, std::memory_order_relaxed);

            // 从 map 移除（heap 中残留等 Run 时清理）
            m_timers.erase(it);
        }

        // 唤醒调度线程
        m_cv.notify_one();
    }

    DeadlineScheduler::DeadlineScheduler(): m_running(true) {
        // 启动调度线程
        m_thread = std::thread([this](){ 
            std::ostringstream oss;
            oss << std::this_thread::get_id();
            LogDebug(u"DeadlineScheduler thread started: stdThreadId={}", oss.str());
            this->Run();
        });
    }

    DeadlineScheduler::~DeadlineScheduler() {
        {
            std::lock_guard lock(m_mutex);
            m_running = false; // 设置停止标志
        }

        m_cv.notify_all(); // 唤醒线程以便退出

        // 等待线程结束
        if (m_thread.joinable()) m_thread.join();
    }

    void DeadlineScheduler::Run() {
        std::unique_lock<std::mutex> lock(m_mutex);
        while(m_running) {
            // 没有任务，等待新任务
            if (m_pendingHeap.empty()) {
                m_cv.wait(lock, [this] { return !m_running || !m_pendingHeap.empty(); });
                continue;
            }

            // 获取最近的任务
            auto node = m_pendingHeap.top();

            // 清理已取消任务
            if (node->cancelled.load(std::memory_order_relaxed)) {
                m_pendingHeap.pop();
                continue;
            }

            // 计算等待时间
            if (Clock::now() < node->expiry) {
                m_cv.wait_until(lock, node->expiry, [this, node] {
                    return !m_running || m_pendingHeap.empty() || m_pendingHeap.top() != node;
                });
                continue;
            }

            // 到期，移出堆
            m_pendingHeap.pop();

            // 如果已经取消，跳过
            if (node->cancelled.load(std::memory_order_acquire)) continue;

            // 一次性任务先从 map 移除
            if (node->interval <= Duration::zero()) m_timers.erase(node->id);

            auto callback = node->callback;
            bool isRepeat = node->interval > Duration::zero();
            
            // 执行回调时不持锁
            lock.unlock();

            try {
                if (!node->cancelled.load(std::memory_order_acquire) && callback) callback();
            } catch (const std::exception& e) {
                // 建议这里加日志，不要完全吞掉
                LogError(u"DeadlineScheduler callback exception: {}", e.what());
            } catch (...) {
                LogError(u"DeadlineScheduler callback unknown exception");
            }

            // 重新锁定继续循环
            lock.lock();

            // 周期任务执行完后，如果没取消，再重新入堆
            if (m_running && isRepeat && !node->cancelled.load(std::memory_order_acquire)) {
                node->expiry = Clock::now() + node->interval;
                m_pendingHeap.push(node);
                m_timers[node->id] = node;
                m_cv.notify_one();
            }
        }

        // 清理所有任务
        while (!m_pendingHeap.empty()) m_pendingHeap.pop();
        m_timers.clear();
    }
    
    DeadlineScheduler::TimerId DeadlineScheduler::AddTimer(const TimePoint& tp, Duration interval, Callback cb) {
        // 构造 执行节点
        auto node = std::make_shared<TimerNode>();
        node->id = m_nextId.fetch_add(1, std::memory_order_relaxed);
        node->expiry = tp;
        node->callback = std::move(cb);
        node->interval = interval;
        node->cancelled.store(false, std::memory_order_relaxed);

        // 加入队列和映射
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_timers[node->id] = node;
            m_pendingHeap.push(node);
        }
    
        // 唤醒调度线程
        m_cv.notify_one();
    
        return node->id;
    }
}