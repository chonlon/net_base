#include "net/tcp/tcp_server.h"
#include "base/print_helper.h"
#include "io/hook.h"

using namespace lon::io;
using namespace lon::net;
void runServer() {
    printDividing("tcp server");
    TcpServer::Ptr server = std::make_shared<TcpServer>([](std::shared_ptr<TcpConnection> connection)
    {
        fmt::print("got connection: local:{}, peer{}\n", connection->getLocalAddr()->toString(), connection->getPeerAddr()->toString());
        connection->send(lon::StringPiece("tcp server message\n"));
        char buf[1024];
        connection->recv(buf, 1024, 0);
        fmt::print("recv:{}", buf);
        connection->send(lon::StringPiece(fmt::format(
            "hi, {}, we are closing", connection->getPeerAddr()->toString())));
        connection->getSocket().close();
    }, nullptr);
    server->setSocketIniter([](Socket& socket)
    {
        socket.setKeepAlive(true);
        socket.setTcpNoDelay(true);
        socket.setReusePort(true);
    });
    server->bind(std::make_shared<IPV4Address>("127.0.0.1", 22225));
    server->startServe();
    // run nc localhost 22225.
}

void runServerBinds() {
    printDividing("tcp server bind");
    TcpServer server([](std::shared_ptr<TcpConnection>){}, nullptr);
    server.bind(std::vector<std::shared_ptr<SockAddress>>{
        std::make_shared<IPV4Address>("127.0.0.1", 22300),
        std::make_shared<IPV4Address>("127.0.0.1", 22301),
        std::make_shared<IPV4Address>("127.0.0.1", 22302),
        std::make_shared<IPV4Address>("127.0.0.1", 22303),
    });

    // run netstat -atp | grep 223.
    sleep(10);
}

int main() {
    lon::io::setHookEnabled(true);
    IOManager::getThreadLocal()->addExecutor(std::make_shared<lon::coroutine::Executor>(runServerBinds));
    IOManager::getThreadLocal()->addExecutor(std::make_shared<lon::coroutine::Executor>(runServer));
    
    IOManager::getThreadLocal()->run();
}