#include "coroutine/scheduler.h"

static auto G_logger = lon::LogManager::getInstance()->getLogger("system");

namespace lon::coroutine {


bool Scheduler::addExecutor(Executor::Ptr executor, size_t index) {
    if (stopping_)
        return false;
    std::lock_guard<Mutex> locker(executors_mutex_);
    if (index == 0) {
        executors_.front().push_back(executor);
    } else {
        // index 可能越界.
        index %= threads_count_;
        executors_[index].push_back(executor);
    }
    return true;
}

void Scheduler::removeExecutor(Executor::Ptr executor) {
    std::lock_guard<Mutex> locker(executors_mutex_);
    for(auto& executor_container : executors_) {
        auto iter = std::find(executor_container.begin(), executor_container.end(), executor);
        if(iter != executor_container.end()) {
            executor_container.erase(iter);
            break;
        }
    }
}

void Scheduler::threadScheduleFunc(size_t index) {
    auto thread_main_executor = Executor::getCurrent();
    {
        String thread_name = "slaver ";
        thread_name.append(std::to_string(index));
        setThreadName(thread_name);
    }

    while (!stop_pending()) {
        Executor::Ptr executor = nullptr;

        {
            std::lock_guard<Mutex> locker(executors_mutex_);
            if (!executors_[index].empty()) {
                //首先尝试从当前线程的任务队列中取出任务
                executor = executors_[index].front();
                executors_[index].pop_front();
            } else {
                // 当前线程的任务为空, 从公共队列取出任务.
                if (!executors_.front().empty()) {
                    executor = executors_.front().front();
                    executors_.front().pop_front();
                }
            }
        }
        if (executor == nullptr) {
            // 当前没有任务.
            if (stopping_)
                break;
            ++idle_thread_count_;
            block_pending_func_();
            --idle_thread_count_;
            continue;
        }

        if (executor->getState() != Executor::State::Terminal &&
            executor->getState() != Executor::State::Aborted) {
            //executor 执行并结束.
            executor->exec();
            // process executor state.
            // if (executor->getState() == Executor::State::HoldUp) {
            //     executors_[index].push_back(executor);
            // }
            // else
            if (executor->getState() == Executor::State::Terminal || executor->getState() == Executor::State::HoldUp) {
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
