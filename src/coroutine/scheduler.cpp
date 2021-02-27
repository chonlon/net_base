#include "coroutine/scheduler.h"

static auto G_logger = lon::LogManager::getInstance()->getLogger("system");

namespace lon::coroutine {
thread_local std::unique_ptr<Scheduler> t_scheduler = nullptr;

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

bool Scheduler::addExecutor(Executor::Ptr executor, size_t index) {
    if (stopping_)
        return false; //拒绝继续添加任务.

    executors_.push_back(executor);
    return true;
}

void Scheduler::removeExecutor(Executor::Ptr executor) {
    auto iter = std::find(executors_.begin(), executors_.end(), executor);
    if (iter != executors_.end())
        executors_.erase(iter);
}

Scheduler* Scheduler::getThreadLocal() {
    if (t_scheduler)
        return t_scheduler.get();

    t_scheduler = std::make_unique<Scheduler>();
    return t_scheduler.get();
}

void Scheduler::threadScheduleFunc() {

    while (!stop_pending()) {
        Executor::Ptr executor = nullptr;
        if (!executors_.empty()) {
            //首先尝试从当前线程的任务队列中取出任务
            executor = executors_.front();
            executors_.pop_front();
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
