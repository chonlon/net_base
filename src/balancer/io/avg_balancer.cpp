#include "balancer/io/avg_balancer.h"

namespace lon::io {

RandomIOBalancer::RandomIOBalancer(size_t threads_count,
                                   bool use_current_thread) {
    managers_.reserve(threads_count);
    threads_.reserve(threads_count);
    for (auto i = 0; i < threads_count; ++i) {
        threads_.emplace_back([&]() {
            auto manager = IOManager::getThreadLocal();
            managers_.emplace_back(manager);
            manager->run();
        });
    }
    if (use_current_thread) {
        managers_.emplace_back(IOManager::getThreadLocal());
    }
}

RandomIOBalancer::~RandomIOBalancer() {
    for (auto& thread : threads_) {
        thread.join();
    }
}

void RandomIOBalancer::schedule(coroutine::Executor::Ptr executor,
                                std::any arg) {
    std::default_random_engine e;
    std::uniform_int_distribution<size_t> u(0, threads_.size() - 1);
    managers_[u(e)]->addRemoteTask(executor);
}

SequenceIOBalancer::SequenceIOBalancer(size_t threads_count,
                                       bool use_current_thread) {
    managers_.reserve(threads_count);
    threads_.reserve(threads_count);
    for (auto i = 0; i < threads_count; ++i) {
        threads_.emplace_back([&]() {
            auto manager = IOManager::getThreadLocal();
            managers_.emplace_back(manager);
            manager->run();
        });
    }
    if (use_current_thread) {
        managers_.emplace_back(IOManager::getThreadLocal());
    }
}

SequenceIOBalancer::~SequenceIOBalancer() {
    for (auto& thread : threads_) {
        thread.join();
    }
}

void SequenceIOBalancer::schedule(coroutine::Executor::Ptr executor,
                                  std::any arg) {
    static size_t count = 0;
    managers_[count++ % threads_.size()]->addRemoteTask(executor);
}
}  // namespace lon::io
