#include "net/socket.h"

#include "io/io_manager.h"
#include "logger.h"
#include "net/tcp/connection.h"

#include <cassert>
#include <fmt/core.h>
#include <netinet/tcp.h>

static auto G_logger = lon::LogManager::getInstance() -> getLogger("system");
namespace lon::net {

Socket::Socket(int _domain, int _type, int _protocol) {
    fd_ = socket(_domain, _type, _protocol);
    if (fd_ == -1) {
        throw ExecFailed(fmt::format("init socket failed err:{}(with errno={})",
                                     std::strerror(errno),
                                     errno));
    }
}

int Socket::bind(lon::net::SockAddress::SharedPtr local_addr) {
    local_address = local_addr;
    if (!local_addr)
        return -1;
    return ::bind(fd_, local_addr->getAddr(), local_addr->getAddrLen());
}

std::unique_ptr<TcpConnection> Socket::accept() const {
    if (fd_ == -1 || local_address == nullptr) {
        return nullptr;
    }
    sa_family_t family          = local_address->getFamily();
    auto [peer_addr, accept_fd] = sockopt::accept(fd_, family);
    Socket accept_socket(accept_fd);
    accept_socket.local_address = local_address;
    return std::make_unique<TcpConnection>(accept_socket, std::move(peer_addr));
}

int Socket::listen(int backlog) const {
    return ::listen(fd_, backlog);
}

std::unique_ptr<TcpConnection> Socket::connect(
    lon::net::SockAddress::UniquePtr peer_addr) const {
    if (fd_ == -1)
        return nullptr;
    if (!peer_addr)
        return nullptr;
    int ret = ::connect(fd_, peer_addr->getAddr(), peer_addr->getAddrLen());
    LON_ERROR_INVOKE_ASSERT(ret != -1,
                            "connect",
                            fmt::format("fd:{}, err:{}(with {}), peer addr:{}",
                                        fd_,
                                        std::strerror(errno),
                                        errno,
                                        peer_addr->toString()),
                            G_logger);
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
}  // namespace lon::net
