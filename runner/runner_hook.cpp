#include "base/print_helper.h"
#include "io/hook.h"
#include "io/io_manager.h"


#include <chrono>
#include <dlfcn.h>
#include <iostream>

template <typename ClockType>
std::chrono::milliseconds::rep getTimeSpanMs(
    const std::chrono::time_point<ClockType>& time_point_begin,
    const std::chrono::time_point<ClockType>& time_point_end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            time_point_end - time_point_begin)
        .count();
}

using namespace lon;
using namespace lon::io;

void run_sleep() {
    CaseMarker marker("run sleep");
    fmt::print("run in executor {}\n",
               coroutine::Executor::getCurrent()->getId());
    auto begin       = std::chrono::steady_clock::now();
    constexpr int s1 = 1;
    sleep(s1);
    auto end = std::chrono::steady_clock::now();
    fmt::print("1--: except ms: {}, actual ms:{}\n",
               1000 * s1,
               getTimeSpanMs(begin, end));


    begin = end;
    struct timespec spec{};
    spec.tv_sec      = 0;
    constexpr int s2 = 30;
    spec.tv_nsec     = 1000 * 1000 * s2;
    nanosleep(&spec, nullptr);
    end = std::chrono::steady_clock::now();
    fmt::print("2--: except ms: {}, actual ms:{}\n",
               s2,
               getTimeSpanMs(begin, end));

}


int main() {
    auto manager = IOManager::getThreadLocal();
    manager->addExecutor(std::make_shared<coroutine::Executor>(&run_sleep));

    manager->run();
}
