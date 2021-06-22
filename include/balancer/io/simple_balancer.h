#pragma once
#include "balancer.h"
#include "../../io/io_manager.h"

namespace lon::io {
/**
 * @brief 直接转发给本线程IOManager的简单调度器.
*/
class SimpleIOBalancer : public IOWorkBalancer
{
public:
    SimpleIOBalancer() = default;
    ~SimpleIOBalancer() override = default;


    SimpleIOBalancer(const SimpleIOBalancer& _other) = delete;

    SimpleIOBalancer(SimpleIOBalancer&& _other) noexcept = delete;

    auto operator=(const SimpleIOBalancer& _other) -> SimpleIOBalancer& = delete
    ;

    auto operator=(SimpleIOBalancer&& _other) noexcept -> SimpleIOBalancer&
    = delete;

    void schedule(coroutine::Executor::Ptr executor, const std::any& arg) override {
        IOManager::getThreadLocal()->addExecutor(executor);
    }
};
}  // namespace lon::io
