#include "net/tcp/tcp_server.h"
#include "base/print_helper.h"
#include "balancer/io/avg_balancer.h"
#include "balancer/io/prio_balancer.h"
#include "io/hook.h"

using namespace lon::io;
using namespace lon::net;

constexpr int loop_time = 8;
constexpr int server_count = 3;

std::array<uint16_t, server_count> server_ports{22225, 22226, 22227};
std::array<const char*, server_count> server_name{"1", "2", "3"};

void runAvg() {
    {
        TcpServer::Ptr server = std::make_shared<TcpServer>([](std::shared_ptr<TcpConnection> connection)
            {
                fmt::print("-random-,current thread: {}\n", lon::getThreadId());
                fmt::print("got connection: local:{}, peer{}\n", connection->getLocalAddr()->toString(), connection->getPeerAddr()->toString());
                connection->send(lon::StringPiece("tcp server message\n"));
                char buf[1024];
                connection->recv(buf, 1024, 0);
                fmt::print("recv:{}", buf);
                connection->send(lon::StringPiece(fmt::format(
                    "hi, {}, we are closing", connection->getPeerAddr()->toString())));
                connection->getSocket().close();
            }, std::make_unique<RandomIOBalancer>(4));
        server->setSocketIniter([](Socket& socket)
            {
                socket.setKeepAlive(true);
                socket.setTcpNoDelay(true);
                socket.setReusePort(true);
            });
        server->bind(std::make_shared<IPV4Address>("127.0.0.1", server_ports[0]));
        server->startServe();
    }

    {
        TcpServer::Ptr server = std::make_shared<TcpServer>([](std::shared_ptr<TcpConnection> connection)
            {
                fmt::print("-sequence-, current thread: {}\n", lon::getThreadId());
                fmt::print("got connection: local:{}, peer{}\n", connection->getLocalAddr()->toString(), connection->getPeerAddr()->toString());
                connection->send(lon::StringPiece("tcp server message\n"));
                char buf[1024];
                connection->recv(buf, 1024, 0);
                fmt::print("recv:{}", buf);
                connection->send(lon::StringPiece(fmt::format(
                    "hi, {}, we are closing", connection->getPeerAddr()->toString())));
                connection->getSocket().close();
            }, std::make_unique<SequenceIOBalancer>(4));
        server->setSocketIniter([](Socket& socket)
            {
                socket.setKeepAlive(true);
                socket.setTcpNoDelay(true);
                socket.setReusePort(true);
            });
        server->bind(std::make_shared<IPV4Address>("127.0.0.1", server_ports[1]));
        server->startServe();
    }
    
}

void runPrio() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    lon::net::Socket socket(fd);
    socket.bind(std::make_shared<lon::net::IPV4Address>("127.0.0.1", server_ports[2]));
    socket.listen();
    socket.setReusePort(true);
    auto prio_balancer = std::make_shared<PrioBlancer>(std::vector<uint8_t>{3,1,1,1});
    std::vector<int> task_prio{1, 0, 0, 0, 0, 3, 2, 2};
    for(auto prio : task_prio) {
        std::shared_ptr<TcpConnection> connection = socket.accept();

        prio_balancer->schedule(std::make_shared<lon::coroutine::Executor>([connection]()
        {
            fmt::print("-prio-, current thread: {}\n", lon::getThreadId());
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
        }), prio);
    }
}

void multiClient() {
    sleep(3);
    for(int server = 0; server <server_count; ++server) {
        for(int loop = 0; loop < loop_time; ++loop) {
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            lon::net::Socket socket(fd);
            auto connection = socket.connect(std::make_unique<IPV4Address>("127.0.0.1", server_ports[server]));
            if(connection == nullptr)
                fmt::print("error, connect failed, with{}\n", std::strerror(errno));
            char buf[1024]{};
            connection->recv(buf, 1024);
            connection->send(lon::StringPiece(fmt::format("hi, {}\n", server_name[server])));
            bzero(buf, 1024);
            connection->recv(buf, 1024);
        }
    }
}

int main() {
    lon::io::setHookEnabled(true);
    IOManager::getThreadLocal()->addExecutor(std::make_shared<lon::coroutine::Executor>(runAvg));
    IOManager::getThreadLocal()->addExecutor(std::make_shared<lon::coroutine::Executor>(runPrio));


    std::thread client_thread([]()
    {
        IOManager::getThreadLocal()->addExecutor(std::make_shared<lon::coroutine::Executor>(multiClient));
        IOManager::getThreadLocal()->run();
    });

    IOManager::getThreadLocal()->run();
}

