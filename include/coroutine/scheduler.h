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
     * @brief 添加executor到运行队列, 应该在Scheduler同一线程中调用.
     * @param executor 
     * @return 如果scheduler正在停止, 那么会拒绝添加任务, 返回false. 添加成功返回true.
    */
    bool addExecutor(Executor::Ptr executor);

    void removeExecutor(Executor::Ptr executor);

    /**
     * @brief 添加executor当待运行队列, 在不同线程中调用安全.
     * @param executor
     * @return 如果添加成功返回true.
    */
    bool addRemoteExecutor(Executor::Ptr executor);


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
        return ready_executors_.size();
    }

    /**
     * @brief 应该只从Scheduler线程访问
    */
    static std::shared_ptr<Scheduler> getThreadLocal();

    /**
     * @brief 应该只从Scheduler线程设置
    */
    static void setThreadLocal(std::shared_ptr<Scheduler> scheduler);
private:
    void threadScheduleFunc();

    bool stop_pending() {
        return stopped_ && stop_pending_func_();
    }

    struct RemoteTaskList
    {
        struct Node
        {
            Executor::Ptr value;
            Node* next;

            Node(Executor::Ptr _value, Node* _next)
                : value{std::move(_value)},
                  next{_next} {
            }
        };
        std::atomic<Node*> head = nullptr;

        bool insertFront(Executor::Ptr executor) {
            auto new_node = new Node(executor, nullptr);
            auto old_head = head.load(std::memory_order_relaxed);
            do{
                new_node->next = old_head;  
            } while(!head.compare_exchange_strong(old_head, new_node, std::memory_order_release, std::memory_order_relaxed));
            
            return old_head == nullptr;
        }

        void scheduleAll(Scheduler* scheduler) {
            if(auto _head = head.exchange(nullptr); _head) {
                while(_head) {
                    scheduler->addExecutor(_head->value);
                    auto temp = _head;
                    _head = _head->next;
                    delete temp;
                }
            }
        }

        bool empty() const {
            return head.load() == nullptr;
        }
    };

private:
    bool stopping_{false};
    bool stopped_{false};
    bool exit_with_tasks_processed{true};
    
    BlockFuncType block_pending_func_{nullptr};
    StopFuncType stop_pending_func_{nullptr};
    
    std::list<Executor::Ptr> ready_executors_{};
    Executor::Ptr scheduler_executor_ = nullptr;

    RemoteTaskList remote_tasks_;
};


} // namespace lon::coroutine
