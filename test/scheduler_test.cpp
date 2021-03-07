#include <string>
#include <unordered_set>
#include <gtest/gtest.h>
#include "config/yaml_convert_def.h"
#include "coroutine/scheduler.h"


#include <algorithm>
#include <fmt/ranges.h>

TEST(CoroutineTest, sequence) {
    //在单独线程环境中, 任务执行顺序应该是完全按照添加顺序进行的.
    std::thread t([](){
        int sequence = 0;
        auto scheduler = lon::coroutine::Scheduler::getThreadLocal();

    #define SEQ_XX(seq_expect)  \
        scheduler->addExecutor(std::make_shared<lon::coroutine::Executor>([&]() \
            { \
                EXPECT_EQ(sequence++, seq_expect); \
                std::cout << "seq:" << sequence << '\n'; \
            }));

            SEQ_XX(0)
            SEQ_XX(1)
            SEQ_XX(2)
            SEQ_XX(3)
            SEQ_XX(4)
            SEQ_XX(5)
    #undef SEQ_XX

            scheduler->addExecutor(std::make_shared<lon::coroutine::Executor>([scheduler]()
                {
                    scheduler->stop();
                }));
        scheduler->run();
    });
    t.join();
}

TEST(CoroutineTest, exception) {
    //在协程函数中抛出的异常会以日志错误形式提醒.
    EXPECT_NO_THROW([]()
        {
            std::thread t([](){
                auto scheduler = lon::coroutine::Scheduler::getThreadLocal();
                scheduler->addExecutor(std::make_shared<lon::coroutine::Executor>([]()
                    {
                        std::cout << "notion: log error for exception is correct\n";
                        throw std::invalid_argument("invalid");
                    }));
                scheduler->addExecutor(std::make_shared<lon::coroutine::Executor>([scheduler]()
                    {
                        scheduler->stop();
                    }));
                scheduler->run();
            });
            t.join();
        }());
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}