#pragma once
#include "coroutine/scheduler.h"
#include <sys/epoll.h>

namespace lon::io
{
    
class IoManager
{
public:
    using EventCallbackType = std::function<void()>;
    // using FdEventDataType = std::pair<int, EventCallbackType>;

    IoManager(size_t thread_count = coroutine::default_thread_count);
    ~IoManager();

    enum EventType
    {
        Read = EPOLLIN,
        Write = EPOLLOUT
    };

    /**
     * @brief 对fd对应的io时间注册回调.
     * @param fd io的fd
     * @param type 读写类型
     * @param callback 事件对应的回调.
     * @exception bad_alloc from vector resize
     * @return 是否注册成功, 如果IOManager已经stop, 继续注册会失败.
    */
    bool registerEvent(int fd, EventType type, EventCallbackType callback);
    void cancelEvent(int fd, EventType type);

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

    int epoll_fd_{ -1 };
    int wakeup_pipe_fd_[2]{-1,-1};
    coroutine::Scheduler scheduler_;
    std::vector<EventCallbackType> fd_callbacks_;
};


} // namespace lon::io
