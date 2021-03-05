#pragma once
#include <cstring>
#include <sys/epoll.h>

namespace lon {
inline int epollOperation(int epfd, int op, uint32_t events, int fd) {
    struct epoll_event ep_event;
    bzero(&ep_event, sizeof(struct epoll_event));
    ep_event.events  = events;
    ep_event.data.fd = fd;
    return epoll_ctl(epfd, op, fd, &ep_event);
}

}
