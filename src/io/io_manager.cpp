#include "io/io_manager.h"


#include "base/epoll_helper.h"
#include "coroutine/executor.h"

#include <fcntl.h>
#include <unistd.h>

namespace lon::io {
thread_local std::shared_ptr<IOManager> t_io_manager = nullptr;

constexpr int epoll_create_size   = 1000;
constexpr int epoll_wait_max_size = 64;
static Logger::ptr G_Logger = LogManager::getInstance()->getLogger("system");

IOManager::IOManager() : scheduler_{} {
    scheduler_.setExitWithTasksProcessed(true);
    scheduler_.setBlockPendingFunc(std::bind(&IOManager::blockPending, this));

    initEpoll();
    initPipe();
}

bool IOManager::registerEvent(int fd,
                              EventType type,
                              coroutine::Executor::Ptr executor,
                              bool call_once) {
    if (stopped)
        return false;
    if (static_cast<size_t>(fd) >= fd_events_.size()) {
        // fd_events_.resize(static_cast<size_t>(fd));

        fd_events_.resize(static_cast<size_t>(fd * 1.5));
    }

    auto epAdd = [fd, type, this]() {
        uint32_t events_dst = fd_events_[fd].registered_events | type;
        if (fd_events_[fd].registered_events) {
            epollMod(fd, events_dst);
        } else {
            epollAdd(fd, events_dst | EPOLLET);
        }
        fd_events_[fd].registered_events = events_dst;
    };

    if (type == Read) {
        // addTo(fd_read_executors_, read_cb_mutex_);

        fd_events_[fd].read_executor  = std::move(executor);
        fd_events_[fd].read_call_once = call_once;
        epAdd();
    } else if (type == Write) {

        fd_events_[fd].write_executor  = std::move(executor);
        fd_events_[fd].write_call_once = call_once;
        epAdd();
    } else {
        return false;
    }
    return true;
}

bool IOManager::hasEvent(int fd, EventType type) {

    if (static_cast<size_t>(fd) >= fd_events_.size())
        return false;
    return fd_events_[fd].registered_events & type;
}

void IOManager::removeEvent(int fd, uint32_t events) {
    if (static_cast<size_t>(fd) >= fd_events_.size() || !(fd_events_[fd].registered_events & events)) {
        return;
    }
    const uint32_t events_dst = ~fd_events_[fd].registered_events & events;


    if (events_dst) {
        epollMod(fd, events_dst);
    } else {
        epollDel(fd);
    }
    if (!(events_dst & Read)) {
        fd_events_[fd].read_executor = nullptr;
    }
    if (!(events_dst & Write)) {
        fd_events_[fd].write_executor = nullptr;
    }
}

bool IOManager::registerTimer(Timer::Ptr timer) {
    if (stopped)
        return false;
    assert(timer->callback);
    timer_manager_.addTimer(std::move(timer));
    return true;
}

void IOManager::cancelTimer(Timer::Ptr timer) {
    timer_manager_.removeTimer(timer);
}

void IOManager::stop() {
    stopped = true;
    scheduler_.stop();
}


std::shared_ptr<IOManager> IOManager::getThreadLocal() {
    if (!t_io_manager)
        t_io_manager = std::make_shared<IOManager>();

    return t_io_manager;
}

void IOManager::setThreadLocal(std::shared_ptr<IOManager> io_manager) {
    t_io_manager = io_manager;
}


void IOManager::initEpoll() {
    epoll_fd_ = epoll_create(epoll_create_size);
    if (epoll_fd_ == -1) {
        LON_LOG_ERROR(G_Logger) << fmt::format(
            "epoll create failed, with error: {}", std::strerror(errno));
        assert(false);
    }
}

void IOManager::initPipe() {
    int ret = pipe(wakeup_pipe_fd_);
    LON_ERROR_INVOKE_ASSERT(ret != -1, "pipe open", "", G_Logger);

    ret = fcntl(wakeup_pipe_fd_[0], F_SETFL, O_NONBLOCK);
    LON_ERROR_INVOKE_ASSERT(ret != -1, "fcntl", "cmd: F_SETFL, op: O_NONBLOCK", G_Logger);

    epollAdd(wakeup_pipe_fd_[0], EPOLLIN | EPOLLOUT);
}

void IOManager::wakeup() {
    while (wake_lock_.test_and_set(std::memory_order_acquire)) {}// acquire lock
    write(wakeup_pipe_fd_[1], "1", 1);
    wake_lock_.clear(std::memory_order_release); // release lock
}

void IOManager::epollAdd(int fd, uint32_t events) const {
    const int ret = epollOperation(epoll_fd_, EPOLL_CTL_ADD, events, fd);
    LON_ERROR_INVOKE_ASSERT(ret != -1, "epoll_ctl", fmt::format("type: add, events:{}, fd:{}", events, fd),G_Logger);
}

void IOManager::epollMod(int fd, uint32_t events) const {
    const int ret = epollOperation(epoll_fd_, EPOLL_CTL_MOD, events, fd);
    LON_ERROR_INVOKE_ASSERT(ret != -1, "epoll_ctl", fmt::format("type: mod, events:{}, fd:{}", events, fd), G_Logger);
}

void IOManager::epollDel(int fd) const {
    const int ret = epollOperation(epoll_fd_, EPOLL_CTL_DEL, EPOLLET, fd);
    LON_ERROR_INVOKE_ASSERT(ret != -1, "epoll_ctl", fmt::format("type: del,  fd:{}", fd), G_Logger);
}

void IOManager::blockPending() {
    int ret = 0;
    struct epoll_event epoll_events[epoll_wait_max_size];

    {
        const int next_interval = static_cast<int>(timer_manager_.getNextInterval());
        ret = epoll_wait(epoll_fd_, epoll_events, epoll_wait_max_size, next_interval);
        LON_ERROR_INVOKE_ASSERT(ret != -1, "epoll_wait", fmt::format("type: add, epoll_wait_max_size:{}, next_interval:{}", epoll_wait_max_size, next_interval), G_Logger);
    }


    {// 执行定时任务.
        auto timers = timer_manager_.takeExpiredTimers();
        for(auto& timer : timers) {
            scheduler_.addExecutor(std::make_shared<coroutine::Executor>(
                std::move(timer->callback)));
        }
    }


    for (int i = 0; i < ret; ++i) {
        const epoll_event ep_event = epoll_events[i];
        if (ep_event.data.fd == wakeup_pipe_fd_[0]) {
            continue;
        } else {
            {// 删除事件.
                uint32_t left_events = fd_events_[ep_event.data.fd].registered_events;

                if (ep_event.events & EPOLLIN) {
                    if (fd_events_[epoll_events->data.fd].read_call_once) {
                        left_events &= ~EPOLLIN;
                    }
                    else {
                        fd_events_[epoll_events->data.fd].read_executor->reuse();
                    }
                }
                if (ep_event.events & EPOLLOUT) {
                    if (fd_events_[epoll_events->data.fd].write_call_once) {
                        left_events &= ~EPOLLOUT;
                    }
                    else {
                        fd_events_[epoll_events->data.fd].write_executor->reuse();
                    }
                }

                if(left_events != fd_events_[ep_event.data.fd].registered_events) {
                    if (left_events) {
                        epollMod(ep_event.data.fd, left_events);
                    }
                    else {
                        epollDel(ep_event.data.fd);
                    }
                    fd_events_[ep_event.data.fd].registered_events = left_events;
                }
                
            }

            // 执行.
            // addExecutor 必定成功.
            if (ep_event.events & EPOLLIN) {
                bool add_ret = scheduler_.addExecutor(
                    fd_events_[ep_event.data.fd].read_executor);

                if (fd_events_[ep_event.data.fd].read_call_once) {
                    fd_events_[ep_event.data.fd].read_executor = nullptr;
                }
                assert(add_ret);
            }
            if (ep_event.events & EPOLLOUT) {
                bool add_ret = scheduler_.addExecutor(
                    fd_events_[ep_event.data.fd].write_executor);

                if (fd_events_[ep_event.data.fd].write_call_once) {
                    fd_events_[ep_event.data.fd].write_executor = nullptr;
                }
                assert(add_ret);
            }
        }
    }
}
}  // namespace lon::io
