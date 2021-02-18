﻿#include "coroutine/scheduler.h"

static auto G_logger = lon::LogManager::getInstance()->getLogger("system");

namespace lon::coroutine {


void Scheduler::addExecutor(Executor::Ptr executor, int32_t index) {
    std::lock_guard<Mutex> locker(executors_mutex_);
    if(index == 0) {
        executors_.front().push(executor);
    } else {
        executors_[index].push(executor);
    }
}

void Scheduler::threadScheduleFunc(int index) {
    auto thread_main_executor = Executor::getCurrent();

    while (!stop_pending()) {
        Executor::Ptr executor = nullptr;

        {
            std::lock_guard<Mutex> locker(executors_mutex_);
            if (!executors_[index].empty()) {
                //首先尝试从当前线程的任务队列中取出任务
                executor = executors_[index].front();
                executors_[index].pop();
            } else {
                // 当前线程的任务为空, 从公共队列取出任务.
                if (!executors_.front().empty()) {
                    executor = executors_.front().front();
                    executors_.front().pop();
                }
            }
        }
        if (executor == nullptr) { // 当前没有任务.
            ++idle_thread_count_;
            block_pending_func_();
            --idle_thread_count_;
            continue;
        }

        

        {
            //executor 执行并结束.

            executor->exec();
            // process executor state.
            if (executor->getState() == Executor::State::HoldUp) {
                executors_[index].push(executor);
            } else if (executor->getState() == Executor::State::Terminal) {
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
