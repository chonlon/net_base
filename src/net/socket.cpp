#include "net/socket.h"

#include "logger.h"
#include "io/io_manager.h"
#include "net/tcp/connection.h"

#include <netinet/tcp.h>
#include <cassert>
#include <fmt/core.h>

auto G_logger = lon::LogManager::getInstance()->getLogger("system");

namespace lon::sockopt {

int connect(int sock_fd, const SockAddress& sock_address) {
    return ::connect(sock_fd, sock_address.getAddr(), sock_address.getSockLen());
}

std::pair<SockAddress::UniquePtr, int>  accept(int sock_fd) {
    sa_family_t type;
    socklen_t length = sizeof(int);

    getsockopt(sock_fd, SOL_SOCKET, SO_TYPE, &type, &length);
    return accept(sock_fd, type);
}

std::pair<SockAddress::UniquePtr, int>  accept(int sock_fd, sa_family_t family) {
    SockAddress::UniquePtr result = nullptr;
    switch (family) {
    case AF_INET:
        result = std::make_unique<IPV4Address>();
        break;
    case AF_INET6:
        result = std::make_unique<IPV6Address>();
        break;
    case AF_UNIX:
        result = std::make_unique<UnixAddress>();
        break;
    default:
        throw std::invalid_argument(fmt::format("invalid family:{}", family));
    }
    socklen_t len = result->getSockLen();
    int ret = ::accept(sock_fd, result->getAddrMutable(), &len);
    LON_ERROR_INVOKE_ASSERT(ret !=-1, accept, fmt::format("sockfd = {}", sock_fd),G_logger);
    return {std::move(result), ret};
}

void shutdownWrite(int sock_fd) {
    int ret = ::shutdown(sock_fd, SHUT_WR);
    LON_ERROR_INVOKE_ASSERT(ret == 0, shutdown, fmt::format("sockfd = {}", sock_fd),G_logger);
}


void setTcpNoDelay(int sock_fd, bool on) {
    int opval = on ? 1 : 0;
    int ret = ::setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &opval, static_cast<socklen_t>(sizeof(opval)));
    LON_ERROR_INVOKE_ASSERT(ret == 0, setsockopt, fmt::format("type tcpnodelay, opt:{}, sockfd = {}", on, sock_fd),G_logger);
}

void setReuseAddr(int sock_fd, bool on) {
    int opval = on ? 1 : 0;
    int ret = ::setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
        &opval, static_cast<socklen_t>(sizeof(opval)));
    LON_ERROR_INVOKE_ASSERT(ret == 0, setsockopt, fmt::format("type reuseaddr, opt:{}, sockfd = {}", on,sock_fd),G_logger);
}

void setReusePort(int sock_fd, bool on) {
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT,
        &optval, static_cast<socklen_t>(sizeof optval));
    LON_ERROR_INVOKE_ASSERT(ret < 0 && on, setsockopt, "reuse port failed",G_logger);
#else
    if (on)
    {
        LON_LOG_ERROR(G_logger) << "reuse port not supported.";
    }
#endif
}

void setKeepAlive(int sock_fd, bool on) {
    int opval = on ? 1 : 0;
    int ret = ::setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE,
        &opval, static_cast<socklen_t>(sizeof opval));
    LON_ERROR_INVOKE_ASSERT(ret == 0, setsockopt, fmt::format("type keepalive, opt:{}, sockfd = {}", on, sock_fd), G_logger);
}

int send(int sock_fd, StringPiece message, int flags) {
    ::send(sock_fd, message.data(), message.size(), flags);
}

int send(int sock_fd, iovec* buffers, size_t length, int flags) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = static_cast<iovec*>(buffers);
    msg.msg_iovlen = length;
    return ::sendmsg(sock_fd, &msg, flags);
}

int sendTo(int sock_fd,
           const void* buffer,
           size_t length,
           const SockAddress* peer_addr,int flags) {
    ::sendto(sock_fd, buffer, length, flags,peer_addr->getAddr(), peer_addr->getSockLen());
}

int sendTo(int sock_fd, StringPiece message, const SockAddress* peer_addr,int flags) {
    sendTo(sock_fd, message.data(), message.size(), peer_addr, flags);
}

int sendTo(int sock_fd,
    iovec* buffers,
    size_t length,
    SockAddress* peer_addr,int flags) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = static_cast<iovec*>(buffers);
    msg.msg_iovlen = length;
    msg.msg_name = reinterpret_cast<void*>(peer_addr->getAddrMutable());
    msg.msg_namelen = peer_addr->getSockLen();
    return ::sendmsg(sock_fd, &msg, flags);
}


int recv(int sock_fd, iovec* buffers, size_t length, int flags) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = static_cast<iovec*>(buffers);
    msg.msg_iovlen = length;
    return ::recvmsg(sock_fd, &msg, flags);
}

int recvFrom(int sock_fd,
             void* buffer,
             size_t length,
             SockAddress* peer_addr, int flags) {
    socklen_t len = peer_addr->getSockLen();
    return ::recvfrom(sock_fd, buffer, length, flags, peer_addr->getAddrMutable(), &len);
}

int recvFrom(int sock_fd,
    iovec* buffers,
    size_t length,
    SockAddress* peer_addr, int flags) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = static_cast<iovec*>(buffers);
    msg.msg_iovlen = length;
    msg.msg_name = peer_addr->getAddrMutable();
    msg.msg_namelen = peer_addr->getSockLen();
    return ::recvmsg(sock_fd, &msg, flags);
}

}

namespace lon::net {

int Socket::bind(SockAddress::SharedPtr local_addr) {
    local_address = local_addr;
    return ::bind(fd_, local_addr->getAddr(), local_addr->getSockLen());
}

std::optional<TcpConnection> Socket::accept() {
    if(fd == -1 || local_address == nullptr) {
        return std::nullopt;
    }
    sa_family_t family = local_address->getFamily();
    auto [peer_addr, accept_fd] = sockopt::accept(fd_, family);
    return TcpConnection(accept_fd, std::move(peer_addr));
}

int Socket::listen(int backlog) {
    return ::listen(fd_, backlog);
}

std::optional<TcpConnection> Socket::connect(SockAddress::UniquePtr peer_addr) {
    if(fd_ == -1) return std::nullopt;
    ::connect(fd_, peer_addr->getAddr(), peer_addr->getSockLen());
    return TcpConnection(fd_, std::move(peer_addr));
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
