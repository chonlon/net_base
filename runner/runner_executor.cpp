#include "coroutine/scheduler.h"

#include <base/print_helper.h>
#include <coroutine/executor.h>
#include <unistd.h>

void simplePrint() {
    std::cout << __FUNCTION__ << '\n';
}

void runExecutor() {
    CaseMarker marker{"run executor"};

    auto current = lon::coroutine::Executor::getCurrent();
    lon::coroutine::Executor::Ptr executor =
        std::make_shared<lon::coroutine::Executor>([&executor]() {
            std::cout << "run in executor" << '\n';
            executor->yield();
            std::cout << "run in executor after yield\n";
            // lon::coroutine::Executor::getCurrent()->yield();
        });
    executor->exec();
    simplePrint();
    executor->exec();
}

void runScheduler() {
    CaseMarker marker{"run scheduler"};
    lon::coroutine::Scheduler scheduler;
    static int i = 0;
    // scheduler.run();
    for(int j = 0 ; j <10; ++j) {
        scheduler.addExecutor(std::make_shared<lon::coroutine::Executor>([]() 
        {
            fmt::print("current: {}, in executor {} at thread {}\n", ++i, lon::coroutine::Executor::getCurrent()->getId(), lon::getThreadId());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }));
        fmt::print("----{}-----\n", i);
    }
    scheduler.run();
}

struct Initer {
    Initer() {
        static int ii = 0;
        i = ++ii;
        std::cout << "cons " << i <<'\n';
    }
    void foo() {
        std::cout << i << '\n';
    }
    int i;
};

thread_local Initer init{};
Initer inittt;

int main() {
    runExecutor();
    runScheduler();
    // std::vector<std::thread> thrs;
    // for(int i=0; i < 10; ++i) {
    //     thrs.emplace_back([](){
    //         init.foo();
    //     });
    // }
    // for(int i = 0; i < 10; ++i) {
    //     thrs[i].join();
    // }
    return 0;
}
