#include "net/socket.h"

#include "logger.h"
#include "io/io_manager.h"
#include "net/tcp/connection.h"

#include <netinet/tcp.h>
#include <cassert>
#include <fmt/core.h>

static auto G_logger = lon::LogManager::getInstance()->getLogger("system");
namespace lon::net {

int Socket::bind(lon::net::SockAddress::SharedPtr local_addr) {
    local_address = local_addr;
    return ::bind(fd_, local_addr->getAddr(), local_addr->getSockLen());
}

std::unique_ptr<TcpConnection> Socket::accept() const {
    if(fd_ == -1 || local_address == nullptr) {
        return nullptr;
    }
    sa_family_t family = local_address->getFamily();
    auto [peer_addr, accept_fd] = sockopt::accept(fd_, family);
    Socket accept_socket(accept_fd);
    accept_socket.local_address = local_address;
    return  std::make_unique<TcpConnection>(accept_socket, std::move(peer_addr));
}

int Socket::listen(int backlog) const {
    return ::listen(fd_, backlog);
}

std::unique_ptr<TcpConnection> Socket::connect(lon::net::SockAddress::UniquePtr peer_addr) const {
    if(fd_ == -1) return nullptr;
    ::connect(fd_, peer_addr->getAddr(), peer_addr->getSockLen());
    return std::make_unique<TcpConnection>(Socket(*this), std::move(peer_addr));
}

void Socket::shutdownWrite() const {
    sockopt::shutdownWrite(fd_);
    stopWrite();
}

void Socket::setTcpNoDelay(bool on) const {
    sockopt::setTcpNoDelay(fd_, true);
}

void Socket::setReuseAddr(bool on) const {
    sockopt::setReuseAddr(fd_, on);
}

void Socket::setReusePort(bool on) const {
    sockopt::setReusePort(fd_, on);
}

void Socket::setKeepAlive(bool on) const {
    sockopt::setKeepAlive(fd_, on);
}

void Socket::stopRead() const {
    io::IOManager::getThreadLocal()->removeEvent(fd_, io::IOManager::Read);
}

void Socket::stopWrite() const {
    io::IOManager::getThreadLocal()->removeEvent(fd_, io::IOManager::Write);
}

void Socket::close() {
    ::close(fd_);
    fd_ = -1;
}

bool Socket::setOption(int level,
                       int option,
                       const void* result,
                       socklen_t len) const {
    return ::setsockopt(fd_, level, option, result, len) == 0;
}
}
