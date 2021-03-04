#include "coroutine/scheduler.h"

static auto G_logger = lon::LogManager::getInstance()->getLogger("system");

namespace lon::coroutine {
thread_local std::shared_ptr<Scheduler> t_scheduler = nullptr;

Scheduler::Scheduler() {
    Executor::getCurrent();
    block_pending_func_ = []()
    {
        // 默认阻塞0.01s
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
    };
    stop_pending_func_  = []() { return false; };
    scheduler_executor_ = std::make_shared<Executor>(
        std::bind(&Scheduler::threadScheduleFunc, this));
    Executor::setMainExecutor(scheduler_executor_);

    LON_LOG_INFO(G_logger) << "Scheduler construct in thread " << lon::getThreadId();
}

Scheduler::~Scheduler() {
    stop();
    std::cout << "Scheduler destruct in thread " << lon::getThreadIdRaw();
}

bool Scheduler::addExecutor(Executor::Ptr executor) {
    if (UNLIKELY(stopping_))
        return false; //拒绝继续添加任务.

    ready_executors_.push_back(executor);
    return true;
}

void Scheduler::removeExecutor(Executor::Ptr executor) {
    auto iter = std::find(ready_executors_.begin(), ready_executors_.end(), executor);
    if (iter != ready_executors_.end())
        ready_executors_.erase(iter);
}

bool Scheduler::addRemoteExecutor(Executor::Ptr executor) {
    return remote_tasks_.insertFront(executor);
}

std::shared_ptr<Scheduler> Scheduler::getThreadLocal() {
    if (UNLIKELY(!t_scheduler))
        t_scheduler = std::make_unique<Scheduler>();

    return t_scheduler;
}

void Scheduler::setThreadLocal(std::shared_ptr<Scheduler> scheduler) {
    t_scheduler = scheduler;
}

void Scheduler::threadScheduleFunc() {

    while (!stop_pending()) {
        Executor::Ptr executor = nullptr;

        //尝试从其它线程的任务队列中取出任务并加入就绪队列
        remote_tasks_.scheduleAll(this);

        if (!ready_executors_.empty()) {
            //尝试从当前线程的(就绪)任务队列中取出任务
            executor = ready_executors_.front();
            ready_executors_.pop_front();
        }
        if (executor == nullptr) {
            // 当前没有任务.
            if (stopping_)
                break;//停止前所有任务执行结束.
            block_pending_func_();
            continue;
        }
        if (executor->getState() != Executor::State::Terminal &&
            executor->getState() != Executor::State::Aborted) {
            //executor 执行并结束.
            executor->exec();
            
            if (executor->getState() == Executor::State::Terminal || executor->
                getState() == Executor::State::HoldUp) {
            } else if (executor->getState() == Executor::State::Aborted) {
                LON_LOG_WARN(G_logger) << "executor aborted id:" <<
                    executor->getId() << '\n';
            } else {
                LON_LOG_ERROR(G_logger) << "unexpected state, executor id:" <<
                    executor->getId() << '\n';
                assert(false);
            }
        }

    }

}
}
