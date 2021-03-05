#pragma once

#include "balancer.h"
#include "io/io_manager.h"

#include <random>

namespace lon::io {
class PrioBlancer : public IOWorkBalancer
{
public:
    /**
     * @brief 初始化优先级均衡器, 优先级范围应为[0, 65536), 0的优先级最低.
     * @param prio_threads_count 每个优先级对应个线程数量.
     */
    PrioBlancer(std::vector<uint8_t> prio_threads_count);

    /**
     * @param arg 应为int_type
     */
    void schedule(coroutine::Executor::Ptr executor,const std::any& arg) override;

    ~PrioBlancer() override;

private:
    std::vector<std::vector<std::shared_ptr<IOManager>>> prio_workers;
    std::vector<Thread> threads_;
};
}  // namespace lon::io
