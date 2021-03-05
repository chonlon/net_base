#include "base/info.h"
#include "base/print_helper.h"
#include "coroutine/executor.h"
#include "io/hook.h"
#include "io/io_manager.h"
#include "net/address.h"
#include "net/socket.h"
#include "net/tcp/connection.h"

#include <fmt/core.h>

void tcp() {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    printDividing("tcp");
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    lon::net::Socket socket(fd);
    socket.bind(std::make_shared<lon::net::IPV4Address>("127.0.0.1", 22223));
    socket.listen();
    while (true) {
        auto connection = socket.accept();
        fmt::print("socket:{}\n", connection->getSocket().fd());
        fmt::print("local:{}, peer:{}\n",
                   connection->getLocalAddr()->toString(),
                   connection->getPeerAddr()->toString());
        connection->send(lon::StringPiece("tcp send\n"));
        char buf[1024];
        connection->recv(buf, 1024, 0);
        fmt::print("recv:{}", buf);
        connection->send(lon::StringPiece(fmt::format(
            "hi, {}, we are closing", connection->getPeerAddr()->toString())));
        connection->getSocket().close();
    }
}

void udp() {
    printDividing("udp");

    auto recv_addr =
        std::make_shared<lon::net::IPV4Address>("127.0.0.1", 22224);
    auto send_addr = std::make_shared<lon::net::IPV4Address>("127.0.0.1:22226");
    fmt::print("recv addr:{}, send addr:{}\n",
               recv_addr->toString(),
               send_addr->toString());

    lon::io::IOManager::getThreadLocal()->addExecutor(
        std::make_shared<lon::coroutine::Executor>([=]() {  // recv
            int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
            bind(fd, recv_addr->getAddr(), recv_addr->getSockLen());
            char buf[1024];
            lon::sockopt::recvFrom(fd, buf, 1024, send_addr.get(), 0);
            fmt::print("recv:{}\n", buf);
        }));


    lon::io::IOManager::getThreadLocal()->addExecutor(
        std::make_shared<lon::coroutine::Executor>([=]() {  // send
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
            bind(fd, send_addr->getAddr(), send_addr->getSockLen());

            lon::sockopt::sendTo(fd,
                                 fmt::format("your addr is:{}, current time:{}",
                                             recv_addr->toString(),
                                             lon::currentMs()),
                                 recv_addr.get(),
                                 0);
        }));
}

int main() {
    lon::io::setHookEnabled(true);
    lon::io::IOManager::getThreadLocal()->addExecutor(
        std::make_shared<lon::coroutine::Executor>(udp));
    lon::io::IOManager::getThreadLocal()->addExecutor(
        std::make_shared<lon::coroutine::Executor>(tcp));
    lon::io::IOManager::getThreadLocal()->run();
}