#include "coroutine/scheduler.h"

#include <string>
#include <unordered_set>
#include <gtest/gtest.h>
#include "config/yaml_convert_def.h"

#include <algorithm>
#include <fmt/ranges.h>

TEST(CoroutineTest, invoke) {
    auto current = lon::coroutine::Executor::getCurrent();
    int count = 0;
    // 应该交织执行: cur->exe->cur->exe->cur
    lon::coroutine::Executor::Ptr executor =
        std::make_shared<lon::coroutine::Executor>([&]()
        {
            EXPECT_EQ(count, 1);
            ++count;
            executor->yield();
            EXPECT_EQ(count, 3);
            ++count;
        });
    ++count;
    executor->exec();
    EXPECT_EQ(count, 2);
    ++count;
    executor->exec();
    EXPECT_EQ(count, 4);
    //check state
    EXPECT_EQ(executor->getState(), lon::coroutine::Executor::State::Terminal);
}


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}