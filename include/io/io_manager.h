#pragma once
#include "base/timer.h"
#include "coroutine/scheduler.h"
#include <sys/epoll.h>

namespace lon::io
{

/**
 * @brief Io管理入口, 设计工作做单线程环境中.
*/
class IOManager : public Noncopyable
{
public:
    using EventCallbackType = std::function<void()>;
    // using FdEventDataType = std::pair<int, EventCallbackType>;

    IOManager(size_t thread_count = coroutine::default_thread_count);
    ~IOManager() = default;

    enum EventType : uint32_t
    {
        Read = EPOLLIN,
        Write = EPOLLOUT
    };

    /**
     * @brief 对fd对应的io时间注册回调, 线程安全, 如果对应的callback已存在, 那么将会替换已注册的.
     * @param fd io的fd
     * @param type 读写类型
     * @param callback 事件对应的回调.
     * @exception bad_alloc from vector resize
     * @return 是否注册成功, 如果IOManager已经stop或者type错误, 注册会失败.
    */
    bool registerEvent(int fd, EventType type, EventCallbackType callback);
    bool hasEvent(int fd, EventType type);
    void cancelEvent(int fd, uint32_t types);

    /**
     * @brief 注册定时器.
     * @param timer 定时器, 带回调, 注册回调应不为空.
     * @return 是否注册成功, 如果IoManager已经stop, 那么注册会失败.
    */
    bool registerTimer(Timer timer);

    void run() {
        scheduler_.run();
    }

    void stop();

    void setExitWithTasksProcessed(bool _exit_with_tasks_processed) {
        scheduler_.setExitWithTasksProcessed(_exit_with_tasks_processed);
    }
private:
    void initEpoll();
    void initPipe();

    // wake up from epoll_wait if blocking.
    void wakeUpIfBlocking();

    void epollAdd(int fd, uint32_t events) const;
    void epollMod(int fd, uint32_t events) const;
    void epollDel(int fd) const;

    /**
     * @brief run in scheduler slave thread.
    */
    void blockPending();

    bool stopped{false};
    int epoll_fd_{ -1 };// TODO 使每一个线程都分配一个epoll更合理, 但是更复杂.
    int wakeup_pipe_fd_[2]{-1,-1};
    coroutine::Scheduler scheduler_;
    TimerManager timer_manager_;
    Mutex read_cb_mutex_;
    std::vector<EventCallbackType> fd_read_callbacks_;
    Mutex write_cb_mutex_;
    std::vector<EventCallbackType> fd_write_callbacks_;
    Mutex fd_events_mutex_;
    std::vector<uint32_t> fd_events_;
};


} // namespace lon::io
