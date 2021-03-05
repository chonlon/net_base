#pragma once

#include "address.h"
#include "socket_opt.h"

#include <optional>

namespace lon::net {
    class TcpConnection;
    class Socket
    {
    public:
        Socket() = default;
        Socket(int _fd)
            : fd_{_fd} {
        }

        Socket(const Socket& _other) = default;
        Socket(Socket&& _other) noexcept = default;
        auto operator=(const Socket& _other) -> Socket& = default;
        auto operator=(Socket&& _other) noexcept -> Socket& = default;

        LON_ALWAYS_INLINE
        void setFd(int _fd) noexcept {
            fd_ = _fd;
        }

        int bind(SockAddress::SharedPtr local_addr);

        LON_NODISCARD
        std::unique_ptr<TcpConnection> accept() const; //TODO 流式的不止TCP, UnixAddress应该也有.

        int listen(int backlog = SOMAXCONN) const;

        LON_NODISCARD
        std::unique_ptr<TcpConnection> connect(SockAddress::UniquePtr peer_addr) const;

        LON_NODISCARD LON_ALWAYS_INLINE
        int fd() const noexcept {return fd_;}

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
