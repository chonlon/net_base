#include "io/hook.h"

#include "base/macro.h"
#include "io/co_io_function.h"

#include <dlfcn.h>
#include <iostream>
#include <stdarg.h>

bool G_hookEnabled = true;
bool G_hookInited  = false;

void lon::io::setHookEnabled(bool enable) {
    G_hookEnabled = enable;
}

bool lon::io::isHookEnabled() {
    return G_hookEnabled;
}


#define HOOK_FUN(OP) \
    OP(sleep)        \
    OP(usleep)       \
    OP(nanosleep)    \
    OP(socket)       \
    OP(connect)      \
    OP(accept)       \
    OP(read)         \
    OP(readv)        \
    OP(recv)         \
    OP(recvfrom)     \
    OP(recvmsg)      \
    OP(write)        \
    OP(writev)       \
    OP(send)         \
    OP(sendto)       \
    OP(sendmsg)      \
    OP(close)        \
    OP(fcntl)        \
    OP(ioctl)        \
    OP(getsockopt)   \
    OP(setsockopt)

void lon::io::hook_init() {
    if (LIKELY(G_hookInited))
        return;

#define HOOK(name) \
    name##_sys = reinterpret_cast<name##_fun>(dlsym(RTLD_NEXT, #name));
    HOOK_FUN(HOOK)
#undef HOOK

    G_hookInited = true;
}

struct ___HookIniter
{
    ___HookIniter() {
        lon::io::hook_init();
    }
};


extern "C" {
#define FUNC_INIT(name) name##_fun name##_sys = nullptr;
HOOK_FUN(FUNC_INIT)
#undef FUNC_INIT

// sleep
unsigned int sleep(unsigned int seconds) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return sleep_sys(seconds);
    return lon::io::co_sleep(seconds);
}


int usleep(useconds_t usec) {
    lon::io::hook_init();
    return lon::io::co_usleep(usec);
}


int nanosleep(const struct timespec* req, struct timespec* rem) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return nanosleep_sys(req, rem);
    return lon::io::co_nanosleep(req, rem);
}


// socket
int socket(int domain, int type, int protocol) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return socket_sys(domain, type, protocol);
    return lon::io::co_socket(domain, type, protocol);
}


int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return connect_sys(sockfd, addr, addrlen);
    return lon::io::co_connect(sockfd, addr, addrlen);
}


int accept(int s, struct sockaddr* addr, socklen_t* addrlen) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return accept_sys(s, addr, addrlen);
    return lon::io::co_accept(s, addr, addrlen);
}


// read
ssize_t read(int fd, void* buf, size_t count) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return read_sys(fd, buf, count);
    return lon::io::co_read(fd, buf, count);
}


ssize_t readv(int fd, const struct iovec* iov, int iovcnt) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return readv_sys(fd, iov, iovcnt);
    return lon::io::co_readv(fd, iov, iovcnt);
}


ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return recv_sys(sockfd, buf, len, flags);
    return lon::io::co_recv(sockfd, buf, len, flags);
}


ssize_t recvfrom(int sockfd,
                 void* buf,
                 size_t len,
                 int flags,
                 struct sockaddr* src_addr,
                 socklen_t* addrlen) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return recvfrom_sys(sockfd, buf, len, flags, src_addr, addrlen);
    return lon::io::co_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}


ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return recvmsg_sys(sockfd, msg, flags);
    return lon::io::co_recvmsg(sockfd, msg, flags);
}


// write
ssize_t write(int fd, const void* buf, size_t count) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return write_sys(fd, buf, count);
    return lon::io::co_write(fd, buf, count);
}


ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return writev_sys(fd, iov, iovcnt);
    return lon::io::co_writev(fd, iov, iovcnt);
}


ssize_t send(int s, const void* msg, size_t len, int flags) {
    lon::io::hook_init();
    return lon::io::co_send(s, msg, len, flags);
}


ssize_t sendto(int s,
               const void* msg,
               size_t len,
               int flags,
               const struct sockaddr* to,
               socklen_t tolen) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return sendto_sys(s, msg, len, flags, to, tolen);
    return lon::io::co_sendto(s, msg, len, flags, to, tolen);
}


ssize_t sendmsg(int s, const struct msghdr* msg, int flags) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return sendmsg_sys(s, msg, flags);
    return lon::io::co_sendmsg(s, msg, flags);
}


int close(int fd) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return close_sys(fd);
    return lon::io::co_close(fd);
}


int fcntl(int fd, int cmd, ... /* arg */) {
    lon::io::hook_init();
    int ret = 0;
    va_list args;
    va_start(args, cmd);
    ret = lon::io::co_fcntl(fd, cmd, args);
    va_end(args);
    return ret;
}


int ioctl(int d, unsigned long int request, ...) {
    lon::io::hook_init();
    int ret = 0;
    va_list args;
    va_start(args, request);
    ret = lon::io::co_ioctl(d, request, args);
    va_end(args);
    return ret;
}


int getsockopt(
    int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return getsockopt_sys(sockfd, level, optname, optval, optlen);
    return lon::io::co_getsockopt(sockfd, level, optname, optval, optlen);
}


int setsockopt(
    int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
    lon::io::hook_init();
    if (!lon::io::isHookEnabled())
        return setsockopt_sys(sockfd, level, optname, optval, optlen);
    return lon::io::co_setsockopt(sockfd, level, optname, optval, optlen);
}
}
