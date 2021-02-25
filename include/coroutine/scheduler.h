#pragma once
#include <memory>
#include <functional>
#include <list>
#include <atomic>


#include "base/nocopyable.h"
#include "executor.h"
#include "logger.h"


namespace lon::coroutine {


/**
 * @brief 1Thread-->1Scheduler-->N Executor.
*/
class Scheduler : public Noncopyable
{
public:
    
    using BlockFuncType = std::function<void()>;
    using StopFuncType = std::function<bool()>;

    Scheduler();


    ~Scheduler();

    /**
     * @brief 添加executor到指定线程执行
     * @param executor 
     * @param index 指定线程的index, 如果指定线程为0, 则为随机线程执行
     * @return 如果scheduler正在停止, 那么会拒绝添加任务, 返回false. 添加成功返回true.
    */
    bool addExecutor(Executor::Ptr executor, size_t index = 0);

    void removeExecutor(Executor::Ptr executor);


    void run() {
        scheduler_executor_->mainExec();
    }

    void stop() {
        if(exit_with_tasks_processed) {
            stopping_ = true;
        } else {
            stopped_ = true;
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


    LON_NODISCARD auto getExecutorsCount() const -> size_t {
        return executors_.size();
    }


    static Scheduler* getThreadLocal();
private:
    void threadScheduleFunc();

    bool stop_pending() {
        return stopped_ && stop_pending_func_();
    }

private:
    bool stopping_{false};
    bool stopped_{false};
    bool exit_with_tasks_processed{true};
    
    BlockFuncType block_pending_func_{nullptr};
    StopFuncType stop_pending_func_{nullptr};
    
    std::list<Executor::Ptr> executors_{};
    Executor::Ptr scheduler_executor_ = nullptr;
};


} // namespace lon::coroutine
