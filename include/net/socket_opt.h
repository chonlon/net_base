#pragma once

#include "address.h"

#include <optional>

namespace lon::sockopt {
    int connect(int sock_fd, const lon::net::SockAddress& sock_address);

    std::pair<lon::net::SockAddress::UniquePtr, int> accept(int sock_fd);
    std::pair<lon::net::SockAddress::UniquePtr, int> accept(int sock_fd, sa_family_t family);

    void shutdownWrite(int sock_fd);
    void setTcpNoDelay(int sock_fd, bool on);
    void setReuseAddr(int sock_fd, bool on);
    void setReusePort(int sock_fd, bool on);
    void setKeepAlive(int sock_fd, bool on);

    ssize_t send(int sock_fd, StringPiece message, int flags = 0);
    ssize_t send(int sock_fd, iovec* buffers, size_t length, int flags = 0);
    
    ssize_t sendTo(int sock_fd, const void* buffer, size_t length, const lon::net::SockAddress* peer_addr,int flags = 0);
    ssize_t sendTo(int sock_fd, StringPiece message, const lon::net::SockAddress* peer_addr,int flags = 0);
    ssize_t sendTo(int sock_fd, iovec* buffers, size_t length, lon::net::SockAddress* peer_addr,int flags = 0);

    ssize_t recv(int _fd,
        iovec* _iovec,
        size_t _size,
        int _flags);


    ssize_t recvFrom(int sock_fd, void* buffer, size_t length, lon::net::SockAddress* peer_addr, int flags);
    ssize_t recvFrom(int sock_fd, iovec* buffers, size_t length, lon::net::SockAddress* peer_addr);
}