#pragma once
#include "base/nocopyable.h"
#include "coroutine/executor.h"
#include <any>

namespace lon::io {
class IOWorkBalancer : public Noncopyable
{
public:
    virtual ~IOWorkBalancer() = default;
    // virtual void schedule(coroutine::Executor::Ptr) = 0;
    virtual void schedule(coroutine::Executor::Ptr,
                          [[maybe_unused]] std::any arg = std::any()) = 0;
};
}  // namespace lon::io
