#include "base/print_helper.h"
#include "io/hook.h"
#include "io/io_manager.h"
#include <arpa/inet.h>
#include <chrono>
#include <dlfcn.h>
#include <iostream>
#include <sys/socket.h>

constexpr int port = 22222;

using namespace lon;
using namespace lon::io;
using namespace std::chrono_literals;

template <typename ClockType>
std::chrono::milliseconds::rep getTimeSpanMs(
    const std::chrono::time_point<ClockType>& time_point_begin,
    const std::chrono::time_point<ClockType>& time_point_end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               time_point_end - time_point_begin)
        .count();
}


void run_sleep() {
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
    fmt::print(
        "2--: except ms: {}, actual ms:{}\n", s2, getTimeSpanMs(begin, end));
}

void run_server() {
    int socket = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    sockaddr_in addr;
    bzero(&addr, sizeof(sockaddr_in));
    addr.sin_port   = ntohs(port);
    addr.sin_family = AF_INET;
    ::inet_aton("127.0.0.1", &addr.sin_addr);

    ::bind(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr));
    ::listen(socket, 64);

    while (true) {
        sockaddr_in accept_addr;
        socklen_t len = sizeof(sockaddr);
        int accept_fd =
            ::accept(socket, reinterpret_cast<sockaddr*>(&accept_addr), &len);
        fmt::print("connect coming\n");

        IOManager::getThreadLocal()->addExecutor(std::make_shared<coroutine::Executor>([accept_fd](){
            while(true) {
                char buf[1024]{};
                ssize_t nread = 0;

                fmt::print("message coming\n");
                nread = read(accept_fd, buf , 1024);
                if (nread == 0) {
                    // shutdown write.
                    fmt::print("remote closed\n");
                    break;
                } else if (nread == -1 && errno != EAGAIN) {
                    fmt::print("error: {}", std::strerror(errno));
                } else {
                    fmt::print("read: {}\n", buf);
                }
            }
                
        }));
    }
}

void run_client(int index) {
    std::this_thread::sleep_for(5s);
    int socket = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    bzero(&addr, sizeof(sockaddr_in));
    addr.sin_port   = ntohs(port);
    addr.sin_family = AF_INET;
    ::inet_aton("127.0.0.1", &addr.sin_addr);

    if((connect(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr))) < 0) {
        LON_LOG_DEFAULT_ERROR() << "connect error" << " " << std::strerror(errno);
    }
    
    String message = fmt::format("test message in {}\n", index);
    for (int i = 0; i < 10; ++i) {
        //内部调用nanosleep.
        std::this_thread::sleep_for(100ms);
        write(socket, message.c_str(), message.length());
    }
    std::this_thread::sleep_for(10s);
    close(socket);
}

int main() {
    lon::io::setHookEnabled(true);
    auto manager = IOManager::getThreadLocal();
    manager->addExecutor(std::make_shared<coroutine::Executor>(&run_sleep));
    manager->addExecutor(std::make_shared<coroutine::Executor>(&run_server));

    std::thread thread([]() {
        auto _manager = IOManager::getThreadLocal();
        for(int i = 0; i < 3; ++i)
            _manager->addExecutor(std::make_shared<coroutine::Executor>(std::bind(&run_client, i)));
        _manager->run();
    });

    manager->run();
    thread.join();
}
