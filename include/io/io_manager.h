#pragma once
#include "base/timer.h"
#include "coroutine/executor.h"
#include "coroutine/scheduler.h"
#include <sys/epoll.h>

namespace lon::io
{

/**
 * @brief Io管理入口, 设计工作在单线程环境中. 协程实体对应关系是:  1Thread-->IOManager-->1Scheduler-->N Executor.
 * 
*/
class IOManager : public Noncopyable
{
public:
    

    IOManager();
    ~IOManager() = default;

    enum EventType : uint32_t
    {
        Read = EPOLLIN,
        Write = EPOLLOUT
    };

    /**
     * @brief 对fd对应的io事件注册回调, 线程安全, 如果对应的callback已存在, 那么将会替换已注册的.
     * @param fd io的fd
     * @param type 读写类型
     * @param executor 事件对应的执行协程.
     * @exception bad_alloc from vector resize
     * @return 是否注册成功, 如果IOManager已经stop或者type错误, 注册会失败.
    */
    bool registerEvent(int fd, EventType type, coroutine::Executor::Ptr executor);
    bool hasEvent(int fd, EventType type);
    void cancelEvent(int fd, uint32_t types);

    void addExecutor(coroutine::Executor::Ptr executor) {
        scheduler_.addExecutor(executor);
    }

    void removeExecutor(coroutine::Executor::Ptr executor) {
        scheduler_.removeExecutor(executor);
    }

    /**
     * @brief 注册定时器.
     * @param timer 定时器, 带回调, 注册回调应不为空.
     * @return 是否注册成功, 如果IoManager已经stop, 那么注册会失败.
    */
    bool registerTimer(Timer::Ptr timer);

    void cancelTimer(Timer::Ptr timer);

    void run() {
        scheduler_.run();
    }

    void stop();

    void setExitWithTasksProcessed(bool _exit_with_tasks_processed) {
        scheduler_.setExitWithTasksProcessed(_exit_with_tasks_processed);
    }

    /**
     * @brief 获取当前线程的IOManager.
     * @return IOManager指针, 返回不拥有指针所有权, 指针有效期与thread local变量相同.
    */
    static IOManager* getThreadLocal();
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
    int epoll_fd_{ -1 };
    int wakeup_pipe_fd_[2]{-1,-1};
    coroutine::Scheduler scheduler_;
    TimerManager timer_manager_;
    std::vector<coroutine::Executor::Ptr> fd_read_executors_;//fd对应的读事件执行器
    std::vector<coroutine::Executor::Ptr> fd_write_executors_;//fd对应的写事件执行器
    std::vector<uint32_t> fd_events_;//fd 已注册事件类型
};



} // namespace lon::io
