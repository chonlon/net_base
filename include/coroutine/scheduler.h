#pragma once
#include <memory>
#include <functional>
#include <queue>
#include <atomic>


#include "base/nocopyable.h"
#include "executor.h"
#include "logger.h"


namespace lon::coroutine {

const size_t default_thread_count = std::thread::hardware_concurrency() - 1;

class Scheduler : public Noncopyable
{
public:
    using BlockFuncType = std::function<void()>;
    using StopFuncType = std::function<bool()>;

    Scheduler(size_t thread_count = default_thread_count)
        : threads_count_{thread_count ? thread_count : default_thread_count} {
        block_pending_func_ = []()
        {
            // 默认阻塞0.01s
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(10ms);
        };
        stop_pending_func_ = []() { return false; };
        executors_.resize(thread_count + 1);
    }


    ~Scheduler() {
        for (auto& thread : exec_threads_) {
            thread.join();
        }
    }

    /**
     * @brief 添加executor到指定线程执行
     * @param executor 
     * @param index 指定线程的index, 如果指定线程为0, 则为随机线程执行
    */
    void addExecutor(Executor::Ptr executor, int32_t index = 0);

    void run() {
        for (size_t i = 1; i <= threads_count_; ++i) {
            exec_threads_.emplace_back(
                std::bind(&Scheduler::threadScheduleFunc, this, i));
        }
    }


    /**
     * @brief 阻塞函数, 默认为sleep一小段时间. 内部为了执行效率, 没有加锁, 初始化完成后, 应该不再调用此函数.
     * @param _block_pending_func 阻塞函数.
    */
    void setBlockPendingFunc(BlockFuncType _block_pending_func) {
        block_pending_func_ = std::move(_block_pending_func);
    }

    /**
     * @brief 停止条件, 内部为了执行效率, 没有加锁, 初始化完成后, 应该不再调用此函数.
     * @param _stop_pending_func 设置停止条件.
    */
    void setStopPendingFunc(StopFuncType _stop_pending_func) {
        stop_pending_func_ = std::move(_stop_pending_func);
    }


    LON_NODISCARD auto isExitWithTasksProcessed() const -> bool {
        return exit_with_tasks_processed;
    }

    auto setExitWithTasksProcessed(bool _exit_with_tasks_processed) -> void {
        exit_with_tasks_processed = _exit_with_tasks_processed;
    }

    LON_NODISCARD auto getThreadsCount() const -> size_t {
        return threads_count_;
    }

    LON_NODISCARD auto getIdleThreadCount() const -> size_t {
        return idle_thread_count_;
    }

    LON_NODISCARD auto getExecutorsCount() const -> size_t {
        return executors_count_;
    }

private:
    void threadScheduleFunc(int index);

    bool stop_pending() {
        return exit_with_tasks_processed && stop_pending_func_();
    }

private:
    bool exit_with_tasks_processed{true};
    Mutex executors_mutex_{};
    BlockFuncType block_pending_func_{nullptr};
    StopFuncType stop_pending_func_{nullptr};
    std::vector<Thread> exec_threads_{};
    std::vector<std::queue<Executor::Ptr>> executors_{};
    size_t threads_count_;
    std::atomic<size_t> idle_thread_count_ = 0;
    std::atomic<size_t> executors_count_   = 0;
};


} // namespace lon::coroutine
