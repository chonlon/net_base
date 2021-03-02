#pragma once

#include "address.h"

#include <optional>

namespace lon::sockopt {
    using namespace lon::net;
    int connect(int sock_fd, const SockAddress& sock_address);

    std::pair<SockAddress::UniquePtr, int> accept(int sock_fd);
    std::pair<SockAddress::UniquePtr, int> accept(int sock_fd, sa_family_t family);

    void shutdownWrite(int sock_fd);
    void setTcpNoDelay(int sock_fd, bool on);
    void setReuseAddr(int sock_fd, bool on);
    void setReusePort(int sock_fd, bool on);
    void setKeepAlive(int sock_fd, bool on);

    int send(int sock_fd, StringPiece message, int flags = 0);
    int send(int sock_fd, const iovec* buffers, size_t length, int flags = 0);
    
    int sendTo(int sock_fd, const void* buffer, size_t length, const SockAddress* peer_addr,int flags = 0);
    int sendTo(int sock_fd, StringPiece message, const SockAddress* peer_addr,int flags = 0);
    int sendTo(int sock_fd, const iovec* buffers, size_t length, SockAddress* peer_addr,int flags = 0);

    int recv(int _fd,
        iovec* _iovec,
        size_t _size,
        int _flags);


    int recvFrom(int sock_fd, void* buffer, size_t length, SockAddress* peer_addr, int flags);
    int recvFrom(int sock_fd, iovec* buffers, size_t length, SockAddress* peer_addr);

    
}

namespace lon::net {
    class TcpConnection;
    class Socket
    {
    public:
        Socket() = default;
        Socket(int _fd)
            : fd_{_fd} {
        }

        LON_ALWAYS_INLINE
        void setFd(int _fd) noexcept {
            fd_ = _fd;
        }

        int bind(SockAddress::SharedPtr local_addr);
        std::optional<TcpConnection> accept(); //TODO 流式的不止TCP, UnixAddress应该也有.
        int listen(int backlog);
        std::optional<TcpConnection> connect(SockAddress::UniquePtr peer_addr);

        LON_NODISCARD
        int fd() const {return fd_;}

        LON_NODISCARD
        SockAddress::SharedPtr getLocalAddress() const { return local_address; }

        void shutdownWrite() const;
        void setTcpNoDelay(bool on) const;
        void setReuseAddr(bool on) const;
        void setReusePort(bool on) const;
        void setKeepAlive(bool on) const;

        void stopRead() const;
        void stopWrite() const;
        void close();

        bool setOption(int level, int option, const void* result, socklen_t len) const;
        template<class T>
        bool setOption(int level, int option, const T& value) const {
            return setOption(level, option, &value, sizeof(T));
        }
    private:
        int fd_{ -1 };
        SockAddress::SharedPtr local_address = nullptr; // 对于数据报协议只需要本地地址
    };
}
