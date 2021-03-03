#pragma once
#include "balancer.h"
#include "io/io_manager.h"

namespace lon::io {
class RandomIOBalancer : IOWorkBalancer
{
public:
    ~RandomIOBalancer() override;
    void schedule(coroutine::Executor::Ptr) override;

private:

    //TODO use sharedptr.]6
    //TODO finish schedule func.
    std::vector<IOManager> managers_;
};
}
