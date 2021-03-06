#include "balancer/io/avg_balancer.h"
#include "base/random.h"
namespace lon::io {

RandomIOBalancer::RandomIOBalancer(size_t threads_count,
                                   bool use_current_thread) {
    managers_.reserve(threads_count);
    threads_.reserve(threads_count);
    for (size_t i = 0; i < threads_count; ++i) {
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
                                const std::any& arg) {

    managers_[mt19937RandomGen<size_t>(0, threads_.size() -1)]->addRemoteTask(executor);
}

SequenceIOBalancer::SequenceIOBalancer(size_t threads_count,
                                       bool use_current_thread) {
    managers_.reserve(threads_count);
    threads_.reserve(threads_count);
    for (size_t i = 0; i < threads_count; ++i) {
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
                                  const std::any& arg) {
    static size_t count = 0;
    managers_[count++ % threads_.size()]->addRemoteTask(executor);
}
}  // namespace lon::io
