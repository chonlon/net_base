﻿#include "base/print_helper.h"
#include "io.h"
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace lon;
using namespace lon::io;
using namespace std::chrono_literals;
auto G_Logger      = LogManager::getInstance() -> getDefault();
constexpr int port = 22223;

IOManager io_manager;
void runEventServer() {
    int socket = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    sockaddr_in addr;
    bzero(&addr, sizeof(sockaddr_in));
    addr.sin_port   = ntohs(port);
    addr.sin_family = AF_INET;
    ::inet_aton("127.0.0.1", &addr.sin_addr);

    ::bind(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr));
    ::listen(socket, 64);
    io_manager.registerEvent(socket, IOManager::Read, [socket]() {
        fmt::print("connect coming\n");
        sockaddr_in accept_addr;
        socklen_t len = sizeof(sockaddr);
        int accept_fd =
            ::accept(socket, reinterpret_cast<sockaddr*>(&accept_addr), &len);
        fcntl(accept_fd, F_SETFL, O_NONBLOCK);

        io_manager.registerEvent(accept_fd, IOManager::Read, [accept_fd]() {
            char buf[1024]{};
            ssize_t nread = 0;
            ssize_t _len  = 0;
            fmt::print("message coming\n");
            while ((nread = read(accept_fd, buf + _len, 1024)) > 0) {
                _len += nread;
            }
            if (nread == 0) {
                // shutdown write.
                fmt::print("remote closed\n");
            } else if (nread == -1 && errno != EAGAIN) {
                fmt::print("error: {}", std::strerror(errno));
            } else {
                fmt::print("read: {}\n", buf);
            }
        });
    });
}

void runEventClient() {
    int socket = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    bzero(&addr, sizeof(sockaddr_in));
    addr.sin_port   = ntohs(port);
    addr.sin_family = AF_INET;
    ::inet_aton("127.0.0.1", &addr.sin_addr);
    connect(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr));

    String message = "test message\n";
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(100ms);
        write(socket, message.c_str(), message.length());
    }
    std::this_thread::sleep_for(100ms);
    close(socket);
}

void runEvent() {
    CaseMarker marker("run event");
    runEventServer();
    runEventClient();
}

void runTimer(Timer::MsStampType interval) {
    CaseMarker marker("run Timer");
    static int count = 0;
    io_manager.registerTimer(Timer(
        interval,
        []() { std::cout << ++count << " at " << getThreadNameRaw() << '\n'; },
        true));

    std::this_thread::sleep_for(10s);
    io_manager.stop();
    std::cout << "final count is " << count << std::endl;
}


int main() {
    io_manager.run();
    // runTimer(1000);


    runEvent();
    runTimer(100);

    // LON_ERROR_INVOKE_ASSERT(false, "test", G_Logger);


    io_manager.stop();
}