#include "io/co_io_function.h"


#include "io/fd_manager.h"
#include "io/hook.h"
#include "io/io_manager.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <typeinfo>

namespace lon::io {
static Logger::ptr G_Logger = LogManager::getInstance()->getLogger("system");

void sleepInner(unsigned ms) {
    auto current    = coroutine::Executor::getCurrent();
    auto io_manager = IOManager::getThreadLocal();
    io_manager->registerTimer(std::make_shared<Timer>(
        ms, [io_manager, current]() { io_manager->addExecutor(current); }));
    current->yield();
}

// TODO timeout.
template <typename FuncType, typename... Args>
ssize_t ioInner(int fd,
                IOManager::EventType event_type,
                FuncType func,
                Args&&... args) {
    FdContext* context = FdManager::getInstance()->getContext(fd);
    if (!context || !context->is_socket || context->is_user_non_block) {
        return func(fd, std::forward<Args>(args)...);
    } else {
        auto io_manager = IOManager::getThreadLocal();

        size_t time_out_ms = 0;
        if (event_type == IOManager::Read) {
            time_out_ms = context->readTimeout;
        } else if (event_type == IOManager::Write) {
            time_out_ms = context->writeTimeout;
        }
        ssize_t n_bytes = -1;
        while (true) {
            // try to exec func.
            n_bytes = func(fd, std::forward<Args>(args)...);
            while (n_bytes == -1 && errno == EINTR) {
                n_bytes = func(fd, std::forward<Args>(args)...);
            }
            if (n_bytes == -1 && errno == EAGAIN) {
                // exec failed.
                auto time_out_ptr = std::make_shared<bool>(false);
                std::weak_ptr<bool> time_out_weak_ptr = time_out_ptr;

                Timer::Ptr timer = std::make_shared<Timer>(
                    time_out_ms,
                    [io_manager, fd, event_type, time_out_weak_ptr]() {
                        auto is_time_out = time_out_weak_ptr.lock();
                        if (is_time_out && !*is_time_out) {
                            io_manager->removeEvent(fd, event_type);
                            *is_time_out = true;
                        }
                    });
                io_manager->registerTimer(timer);
                io_manager->registerEvent(
                    fd, event_type, coroutine::Executor::getCurrent());
                coroutine::Executor::getCurrent()->yield();
                if (*time_out_ptr) {
                    LON_LOG_WARN(G_Logger)
                        << fmt::format("{} invoke timeout with fd:{}",
                                       typeid(func).name(),
                                       fd);
                    return -1;
                } else {
                    io_manager->cancelTimer(timer);
                }
            } else {
                // 函数执行成功, (nonblock状态的阻塞函数的errno!=EAGAIN).
                break;
            }
        }
        return n_bytes;
    }
}

unsigned co_sleep(unsigned seconds) {
    hook_init();
    sleepInner(seconds * 1000);
    return 0;
}

int co_usleep(useconds_t usec) {
    hook_init();
    sleepInner(usec / 1000);
    return 0;
}

int co_nanosleep(const timespec* req, [[maybe_unused]] timespec* rem) {
    hook_init();
    unsigned ms =
        static_cast<unsigned>(req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000);
    sleepInner(ms);
    return 0;
}

int co_socket(int domain, int type, int protocol) {
    hook_init();
    int fd = socket_sys(domain, type, protocol);
    if (fd == -1)
        return fd;
    int flags = fcntl_sys(fd, F_GETFL, 0);
    if (!(flags & O_NONBLOCK)) {
        fcntl_sys(fd, F_SETFL, flags | O_NONBLOCK);
    }
    FdContext context(true);
    context.is_sys_non_block = true;
    FdManager::getInstance()->setContext(fd, context);
    return fd;
}

int co_connect(int sockfd, const sockaddr* addr, socklen_t addrlen) {
    hook_init();
    auto context = FdManager::getInstance()->getContext(sockfd);
    if (!context) {
        errno = EBADF;
        return -1;
    }

    if (!context->is_socket || context->is_user_non_block) {
        return connect_sys(sockfd, addr, addrlen);
    }

    {
        int ret = connect_sys(sockfd, addr, addrlen);
        if (ret != -1 || errno != EINPROGRESS) {
            return ret;
        }
    }
    // 下面意味着sockfd是非阻塞的, 并且没有成功连接,
    // 那么协程主动让出执行权限(直到epoll触发).
    auto io_manager = IOManager::getThreadLocal();
    io_manager->registerEvent(
        sockfd, IOManager::Write, coroutine::Executor::getCurrent());
    coroutine::Executor::getCurrent()->yield();

    // yield返回的时候, 说明connect执行完成(失败).
    int error     = 0;
    socklen_t len = sizeof(int);
    if (-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if (!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int co_accept(int s, sockaddr* addr, socklen_t* addrlen) {
    hook_init();
    int fd = static_cast<int>(
        ioInner(s, IOManager::Read, accept_sys, addr, addrlen));
    if (fd >= 0) {
        int flags = fcntl_sys(fd, F_GETFL, 0);
        if (!(flags & O_NONBLOCK)) {
            fcntl_sys(fd, F_SETFL, flags | O_NONBLOCK);
        }
        FdContext context(true);
        context.is_sys_non_block = true;
        FdManager::getInstance()->setContext(fd, context);
    }
    return fd;
}

ssize_t co_read(int fd, void* buf, size_t count) {
    hook_init();
    return ioInner(fd, IOManager::Read, read_sys, buf, count);
}

ssize_t co_readv(int fd, const iovec* iov, int iovcnt) {
    hook_init();
    return ioInner(fd, IOManager::Read, readv_sys, iov, iovcnt);
}

ssize_t co_recv(int sockfd, void* buf, size_t len, int flags) {
    hook_init();
    return ioInner(sockfd, IOManager::Read, recv_sys, buf, len, flags);
}

ssize_t co_recvfrom(int sockfd,
                    void* buf,
                    size_t len,
                    int flags,
                    sockaddr* src_addr,
                    socklen_t* addrlen) {
    hook_init();
    return ioInner(sockfd,
                   IOManager::Read,
                   recvfrom_sys,
                   buf,
                   len,
                   flags,
                   src_addr,
                   addrlen);
}

ssize_t co_recvmsg(int sockfd, msghdr* msg, int flags) {
    hook_init();
    return ioInner(sockfd, IOManager::Read, recvmsg_sys, msg, flags);
}

ssize_t co_write(int fd, const void* buf, size_t count) {
    hook_init();
    return ioInner(fd, IOManager::Write, write_sys, buf, count);
}

ssize_t co_writev(int fd, const iovec* iov, int iovcnt) {
    hook_init();
    return ioInner(fd, IOManager::Write, writev_sys, iov, iovcnt);
}

ssize_t co_send(int s, const void* msg, size_t len, int flags) {
    hook_init();
    return ioInner(s, IOManager::Write, send_sys, msg, len, flags);
}

ssize_t co_sendto(int s,
                  const void* msg,
                  size_t len,
                  int flags,
                  const sockaddr* to,
                  socklen_t tolen) {
    hook_init();
    return ioInner(s, IOManager::Write, sendto_sys, msg, len, flags, to, tolen);
}

ssize_t co_sendmsg(int s, const msghdr* msg, int flags) {
    hook_init();
    return ioInner(s, IOManager::Write, sendmsg_sys, msg, flags);
}

int co_close(int fd) {
    hook_init();
    if (FdManager::getInstance()->hasFd(fd)) {
        IOManager::getThreadLocal()->removeEvent(
            fd, IOManager::Read | IOManager::Write);
        FdManager::getInstance()->delContext(fd);
    }
    return close_sys(fd);
}

int co_fcntl(int fd, int cmd, ...) {
    hook_init();
    va_list vas;
    va_start(vas, cmd);
    switch (cmd) {
        case F_SETFL: {
            int arg1 = va_arg(vas, int);
            va_end(vas);
            auto context = FdManager::getInstance()->getContext(fd);
            if (!context || context->is_socket) {
                return fcntl_sys(fd, F_SETFL, arg1);
            }
            // 对于socket类型, 用户自己如果需要O_NONBLOCK,
            // 那么在IO操作的使用会使用系统原本api.
            context->is_user_non_block = arg1 & O_NONBLOCK;
            // 对于socket类型, 强制为 O_NONBLOCK(system)
            context->is_sys_non_block = true;
            arg1 |= O_NONBLOCK;
            return fcntl_sys(fd, F_SETFL, arg1);
        }

        break;
        case F_GETFL: {
            va_end(vas);
            int raw_result = fcntl_sys(fd, F_GETFL);
            auto context   = FdManager::getInstance()->getContext(fd);
            if (!context || context->is_socket) {
                return raw_result;
            }
            if (context->is_user_non_block) {
                raw_result |= O_NONBLOCK;
            } else {
                raw_result &= ~O_NONBLOCK;
            }
            return raw_result;
        } break;
        case F_DUPFD:
            [[fallthrough]];
        case F_DUPFD_CLOEXEC:
            [[fallthrough]];
        case F_SETFD:
            [[fallthrough]];
        case F_SETOWN:
            [[fallthrough]];
        case F_SETSIG:
            [[fallthrough]];
        case F_SETLEASE:
            [[fallthrough]];
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int arg = va_arg(vas, int);
            va_end(vas);
            return fcntl_sys(fd, cmd, arg);
        } break;
        case F_GETFD:
            [[fallthrough]];
        case F_GETOWN:
            [[fallthrough]];
        case F_GETSIG:
            [[fallthrough]];
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            va_end(vas);
            return fcntl_sys(fd, cmd);
        } break;
        case F_SETLK:
            [[fallthrough]];
        case F_SETLKW:
            [[fallthrough]];
        case F_GETLK: {
            struct flock* arg = va_arg(vas, struct flock*);
            va_end(vas);
            return fcntl_sys(fd, cmd, arg);
        } break;
        case F_GETOWN_EX:
            [[fallthrough]];
        case F_SETOWN_EX: {
            struct f_owner_exlock* arg = va_arg(vas, struct f_owner_exlock*);
            va_end(vas);
            return fcntl_sys(fd, cmd, arg);
        } break;
        default:
            va_end(vas);
            return fcntl_sys(fd, cmd);
    }
}

int co_ioctl(int d, unsigned long request, ...) {
    hook_init();
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if (FIONBIO == request) {
        bool user_nonblock = !!*static_cast<int*>(arg);
        auto context       = FdManager::getInstance()->getContext(d);
        if (!context || !context->is_socket) {
            return ioctl_sys(d, request, arg);
        }
        context->is_user_non_block = user_nonblock;
    }
    return ioctl_sys(d, request, arg);
}

int co_getsockopt(
    int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
    hook_init();
    return getsockopt_sys(sockfd, level, optname, optval, optlen);
}

int co_setsockopt(
    int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
    hook_init();
    if (level == SOL_SOCKET) {
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            auto context = FdManager::getInstance()->getContext(sockfd);
            if (context) {
                const timeval* v = static_cast<const timeval*>(optval);
                long timeout     = v->tv_sec * 1000 + v->tv_usec / 1000;
                if (optname == SO_RCVTIMEO) {
                    context->readTimeout = timeout;
                } else {
                    context->writeTimeout = timeout;
                }
            }
        }
    }
    return setsockopt_sys(sockfd, level, optname, optval, optlen);
}
}  // namespace lon::io
