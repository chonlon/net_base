#pragma once
#include "balancer.h"
#include "io/io_manager.h"

namespace lon::io {
class SimpleIOBalancer : public IOWorkBalancer
{
public:
    ~SimpleIOBalancer() override {}
    void schedule(coroutine::Executor::Ptr executor, std::any arg) override {
        IOManager::getThreadLocal()->addExecutor(executor);
    }
};
}  // namespace lon::io
