#include "balancer/io/prio_balancer.h"

#include "base/random.h"

namespace lon::io {

PrioBlancer::PrioBlancer(std::vector<uint8_t> prio_threads_count) {
    prio_workers.resize(prio_threads_count.size());
    for (size_t i = 0; i < prio_threads_count.size(); ++i) {
        for (size_t count = 0; count < prio_threads_count[i]; ++count) {
            threads_.emplace_back([this, i]() {
                    auto manager = IOManager::getThreadLocal();
                    prio_workers[i].push_back(manager);
                    manager->run();
                });
        }
    }
}

void PrioBlancer::schedule(coroutine::Executor::Ptr executor, const std::any& arg) {
    int prio;
    try {
        prio = std::any_cast<int>(arg);
    } catch (std::bad_any_cast&) {
        LON_LOG_ERROR(LogManager::getInstance()->getLogger("system")) << fmt::format("convert from arg to size_t failed in PrioBalancer");
        return;
    }

    //超过注册的最大优先级, 降级执行.
    if (prio > static_cast<int>(prio_workers.size() - 1))
        prio = static_cast<int>(prio_workers.size() - 1);
    //当前优先级没有线程, 降级执行.
    while (prio_workers[prio].size() == 0 && prio >= 0) {
        --prio;
    }

    if (prio == -1) {
        LON_LOG_ERROR(LogManager::getInstance()->getLogger("system")) << fmt::format("no valid IOManager to exec.");
        return;
    }

    {
        //随机分配给对应优先级的线程执行.
        prio_workers[prio][mt19937RandomGen<size_t>(0,
            prio_workers[prio].size() - 1)]->addRemoteTask(executor);
    }
}

PrioBlancer::~PrioBlancer() {
    for (auto& thread : threads_) {
        thread.join();
    }
}
}  // namespace lon::io
