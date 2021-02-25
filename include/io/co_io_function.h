#pragma once
#include <iostream>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * @brief io函数的协程版本.
 * 
 */

namespace lon::io {
// sleep
unsigned int co_sleep(unsigned int seconds);


int co_usleep(useconds_t usec);


int co_nanosleep(const struct timespec* req, struct timespec* rem);


// socket

/**
 * @brief socket call wrapper, 隐式设置non block.
*/
int co_socket(int domain, int type, int protocol);


int co_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);


int co_accept(int s, struct sockaddr* addr, socklen_t* addrlen);

// read
ssize_t co_read(int fd, void* buf, size_t count);


ssize_t co_readv(int fd, const struct iovec* iov, int iovcnt);


ssize_t co_recv(int sockfd, void* buf, size_t len, int flags);


ssize_t co_recvfrom(int sockfd,
                    void* buf,
                    size_t len,
                    int flags,
                    struct sockaddr* src_addr,
                    socklen_t* addrlen);


ssize_t co_recvmsg(int sockfd, struct msghdr* msg, int flags);


// write
ssize_t co_write(int fd, const void* buf, size_t count);


ssize_t co_writev(int fd, const struct iovec* iov, int iovcnt);


ssize_t co_send(int s, const void* msg, size_t len, int flags);


ssize_t co_sendto(int s,
                  const void* msg,
                  size_t len,
                  int flags,
                  const struct sockaddr* to,
                  socklen_t tolen);


ssize_t co_sendmsg(int s, const struct msghdr* msg, int flags);


int co_close(int fd);

/**
 * @brief 主要是为了给socket类型的fd隐式加上non block.
*/
int co_fcntl(int fd, int cmd, ... /* arg */);


int co_ioctl(int d, unsigned long int request, ...);


int co_getsockopt(
    int sockfd,
    int level,
    int optname,
    void* optval,
    socklen_t* optlen);


int co_setsockopt(
    int sockfd,
    int level,
    int optname,
    const void* optval,
    socklen_t optlen);
}  // namespace lon::io
