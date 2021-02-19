#include "io.h"
#include "base/print_helper.h"

using namespace lon;
using namespace lon::io;
using namespace std::chrono_literals;
auto G_Logger = LogManager::getInstance()->getDefault();

IOManager io_manager;

void runTimer(Timer::MsStampType interval) {
    CaseMarker marker("run Timer");
    static int count = 0;
    io_manager.registerTimer(Timer(interval, [](){ std::cout << ++count <<  getThreadNameRaw() << '\n'; }, true));
    io_manager.run();
    std::this_thread::sleep_for(10s);
    io_manager.stop();
    std::cout << "final count is " << count << std::endl;
}

int main() {
    // setThreadName("hello 123");
    // std::cout << getThreadName();
    // runTimer(1000);
    runTimer(100);

    // LON_ERROR_INVOKE_ASSERT(false, "test", G_Logger);
}