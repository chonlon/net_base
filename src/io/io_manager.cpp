#include "io/io_manager.h"


#include "base/epoll_helper.h"
#include "coroutine/executor.h"

#include <unistd.h>
#include <fcntl.h>

namespace lon::io {
constexpr int epoll_create_size = 1000;
constexpr int epoll_wait_max_size = 64;
Logger::ptr G_Logger = LogManager::getInstance()->getLogger("system");

IoManager::IoManager(size_t thread_count)
    : scheduler_{thread_count} {
    scheduler_.setExitWithTasksProcessed(true);
    scheduler_.setBlockPendingFunc(std::bind(&blockPending, this));

    initEpoll();
    initPipe();
}

bool IoManager::registerEvent(int fd,
                              EventType type,
                              EventCallbackType callback) noexcept {
    if (stopped)
        return false;
    if (static_cast<size_t>(fd) > fd_events_.size()) {
        // fd_events_.resize(static_cast<size_t>(fd));
        {
            std::lock_guard<Mutex> lock_guard(fd_events_mutex_);
            fd_events_.resize(static_cast<size_t>(fd));
        }
        {
            std::lock_guard<Mutex> lock_guard(read_cb_mutex_);
            fd_read_callbacks_.resize(static_cast<size_t>(fd));
        }
        {
            std::lock_guard<Mutex> lock_guard(write_cb_mutex_);
            fd_write_callbacks_.resize(static_cast<size_t>(fd));
        }
    }

    // auto addTo = [&callback, fd](std::vector<EventCallbackType>& _dst,
    //                              Mutex& mutex)
    // {
    //     std::lock_guard<Mutex> lock_guard(mutex);
    //     // if (static_cast<size_t>(fd) > _dst.size()) {
    //     //     _dst.resize(static_cast<size_t>(fd));
    //     // }
    //     // if(_dst[fd] == nullptr) epollAdd(fd, type | EPOLLET);
    //     // else epollMod(fd, type);
    //     _dst[fd] = std::move(callback);
    // };

    auto epAdd = [fd, type, this]()
    {
        std::lock_guard<Mutex> lock_guard(fd_events_mutex_);
        uint32_t events_dst = fd_events_[fd] | type;
        if(fd_events_[fd]) {
            epollMod(fd, events_dst);
        } else {
            epollAdd(fd, events_dst);
        }
    };
    if (type == Read) {
        // addTo(fd_read_callbacks_, read_cb_mutex_);
        std::lock_guard<Mutex> lock_guard(read_cb_mutex_);
        fd_read_callbacks_[fd] = std::move(callback);
        epAdd();
    } else if (type == Write) {
        std::lock_guard<Mutex> lock_guard(write_cb_mutex_);
        fd_write_callbacks_[fd] = std::move(callback);
        epAdd();
    } else {
        return false;
    }
    return true;
}

bool IoManager::hasEvent(int fd, EventType type) {
    //
    // if(type == Read) {
    //     if(static_cast<size_t>(fd) > fd_read_callbacks_.size()) return false;
    //     return fd_read_callbacks_[fd] == nullptr;
    // } else if(type == Write) {
    //     if (static_cast<size_t>(fd) > fd_write_callbacks_.size()) return false;
    //     return fd_write_callbacks_[fd] == nullptr;
    // } else {
    //     return false;
    // }
    std::lock_guard<Mutex> lock_guard(fd_events_mutex_);
    if (static_cast<size_t>(fd) > fd_events_.size()) return false;
    return fd_events_[fd] & type;
}

void IoManager::cancelEvent(int fd, uint32_t type) {
    if (static_cast<size_t>(fd) > fd_events_.size()) {
        return;
    }

    std::lock_guard<Mutex> lock_guard(fd_events_mutex_);
    uint32_t events_dst = ~fd_events_[fd] & type;
    fd_events_mutex_.unlock();

    if (events_dst) {
        epollMod(fd, events_dst);
    }
    else {
        epollDel(fd);
    }
    if(!(events_dst & Read) && hasEvent(fd, Read)) { //read的callback已注册, 并且需要清除的事件有read, 则将read的callback删除.
        std::lock_guard<Mutex> lock_guard(read_cb_mutex_);
        fd_read_callbacks_[fd] = nullptr;
    }
    if(!(events_dst & Write) && hasEvent(fd, Write)) { // 同上read.
        std::lock_guard<Mutex> lock_guard(write_cb_mutex_);
        fd_write_callbacks_[fd] = nullptr;
    }
}

void IoManager::stop() {
    stopped = true;
    scheduler_.stop();
}

void IoManager::initEpoll() {
    epoll_fd_ = epoll_create(epoll_create_size);
    if (epoll_fd_ == -1) {
        LON_LOG_ERROR(G_Logger) << fmt::format(
            "epoll create failed, with error: {}",
            std::strerror(errno));
        assert(false);
    }
}

void IoManager::initPipe() {
    int ret = pipe(wakeup_pipe_fd_);
    LON_ERROR_INVOKE_ASSERT(ret != -1, "pipe open", G_Logger);

    ret = fcntl(wakeup_pipe_fd_[0], F_SETFL, O_NONBLOCK);
    LON_ERROR_INVOKE_ASSERT(ret != -1, "fcntl", G_Logger);

    epollAdd(wakeup_pipe_fd_[0], EPOLLIN | EPOLLOUT);
}

void IoManager::wakeUpIf() {
    if (scheduler_.getIdleThreadCount() != 0)
        write(wakeup_pipe_fd_[1], "1", 1);
}

void IoManager::epollAdd(int fd, uint32_t events) const {
    const int ret = epollOperation(epoll_fd_, EPOLL_CTL_ADD, events, fd);
    LON_ERROR_INVOKE_ASSERT(ret != -1, "epoll_ctl", G_Logger);
}

void IoManager::epollMod(int fd, uint32_t events) const {
    const int ret = epollOperation(epoll_fd_, EPOLL_CTL_MOD, events,fd);
    LON_ERROR_INVOKE_ASSERT(ret != -1, "epoll_ctl", G_Logger);
}

void IoManager::epollDel(int fd) {
    const int ret = epollOperation(epoll_fd_, EPOLL_CTL_DEL, EPOLLET, fd);
    LON_ERROR_INVOKE_ASSERT(ret != -1, "epoll_ctl", G_Logger);
}

void IoManager::blockPending() {

    struct epoll_event epoll_events[epoll_wait_max_size];
    int ret = epoll_wait(epoll_fd_, epoll_events, epoll_wait_max_size, 0);
    for (int i = 0; i < ret; ++i) {
        auto ep_event = epoll_events[i];
        if (ep_event.data.fd == wakeup_pipe_fd_[0]) {
            continue;
        } else {
            //addExecutor 必定成功.
            if (ep_event.events == EPOLLIN) {
                std::lock_guard<Mutex> lock_guard(read_cb_mutex_);
                bool add_ret = scheduler_.addExecutor(
                    std::make_shared<coroutine::Executor>(
                        fd_read_callbacks_[epoll_events->data.fd])
                    );
                assert(add_ret);
            } else {
                std::lock_guard<Mutex> lock_guard(write_cb_mutex_);
                bool add_ret = scheduler_.addExecutor(
                    std::make_shared<coroutine::Executor>(
                        fd_write_callbacks_[epoll_events->data.fd])
                    );
                assert(add_ret);
            }
        }
    }


}
}
