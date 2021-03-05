#include "net/address.h"
#include "net/socket.h"
#include "net/tcp/connection.h"
#include "io/hook.h"

#include <fmt/core.h>

void tcp() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    lon::net::Socket socket(fd);
    socket.bind(std::make_shared<lon::net::IPV4Address>("127.0.0.1", 22223));
    socket.listen();
    while(true) {
        auto connection = socket.accept();
        fmt::print("socket:{}\n", connection->getSocket().fd());
        fmt::print("local:{}, peer:{}\n", connection->getLocalAddr()->toString(), connection->getPeerAddr()->toString());
        connection->send(lon::StringPiece("tcp send\n"));
        char buf[1024];
        connection->recv(buf, 1024, 0);
        fmt::print("recv:{}", buf);
        connection->send(lon::StringPiece(fmt::format("hi, {}, we are closing", connection->getPeerAddr()->toString())));
        connection->getSocket().close();
    }
    
}

int main() {
    lon::io::setHookEnabled(true);
    tcp();
}