#pragma once
#include "../../base/nocopyable.h"
#include "../../coroutine/executor.h"
#include <any>
#include <iostream>

namespace lon::io {
class IOWorkBalancer
{
public:
    IOWorkBalancer() = default;
    virtual ~IOWorkBalancer() = default;

    /**
     * @brief schedule task.
     * @param executor 等待调度的任务.
     * @param arg 可选参数.
    */
    virtual void schedule(coroutine::Executor::Ptr executor,
                          [[maybe_unused]] const std::any& arg = std::any()) = 0;
};
}  // namespace lon::io
