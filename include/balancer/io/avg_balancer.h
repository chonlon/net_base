#pragma once
#include "balancer.h"
#include "io/io_manager.h"

#include <random>

namespace lon::io {
/**
 * @brief ���ƽ�����������
*/
class RandomIOBalancer : IOWorkBalancer
{
public:
    RandomIOBalancer(
        size_t threads_count    = std::thread::hardware_concurrency() - 1,
        bool use_current_thread = false);


    ~RandomIOBalancer() override;

    void schedule(coroutine::Executor::Ptr executor, std::any arg) override;


private:

    std::vector<std::shared_ptr<IOManager>> managers_;
    size_t balance_thread_count_;
    std::vector<Thread> threads_;
};


/**
 * @brief ˳��ƽ�����������
*/
class SequenceIOBalancer : IOWorkBalancer
{
public:
    SequenceIOBalancer(
        size_t threads_count    = std::thread::hardware_concurrency() - 1,
        bool use_current_thread = false);


    ~SequenceIOBalancer() override;

    void schedule(coroutine::Executor::Ptr executor, std::any arg) override;

private:

    std::vector<std::shared_ptr<IOManager>> managers_;
    size_t balance_thread_count_;
    std::vector<Thread> threads_;
};

}
