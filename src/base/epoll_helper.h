#pragma once
#include <cstring>
#include <sys/epoll.h>
#include <type_traits>

namespace lon {
inline int epollOperation(int epfd, int op, uint32_t events, int fd) {
    struct epoll_event ep_event;
    bzero(&ep_event, sizeof(struct epoll_event));
    ep_event.events  = events;
    ep_event.data.fd = fd;
    return epoll_ctl(epfd, op, fd, &ep_event);
}


/**
 * @brief 只对integral返回类型的调用有效, 需要满足会在被信号中断时的时候返回-1并且errno==EINTR.
 * @return -1 for fail, other for success, as same as original func.
*/
template <typename FuncType, typename... Args, typename Ret = std::invoke_result_t<FuncType, Args...>>
    std::enable_if_t<std::is_integral<std::decay_t<Ret>>::value,Ret>
    invokeNoIntr(FuncType&& func, Args&&... args) {
retry:
    Ret ret = func(std::forward<Args>(args)...);
    if (ret == -1 && errno == EINTR)
        goto retry;
    return ret;
}
}
