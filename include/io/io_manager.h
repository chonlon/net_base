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
     * @param call_once if true, 注册事件协程执行一次即结束, if false, 注册事件协程像回调一样会在每次事件触发时执行.
     * @exception bad_alloc from vector resize
     * @return 是否注册成功, 如果IOManager已经stop或者type错误, 注册会失败.
    */
    bool registerEvent(int fd, EventType type, coroutine::Executor::Ptr executor, bool call_once = true);
    bool hasEvent(int fd, EventType type);
    void removeEvent(int fd, uint32_t events);

    void addExecutor(coroutine::Executor::Ptr executor) {
        scheduler_.addExecutor(executor);
    }

    void removeExecutor(coroutine::Executor::Ptr executor) {
        scheduler_.removeExecutor(executor);
    }

    void addRemoteTask(coroutine::Executor::Ptr executor) {
        scheduler_.addRemoteExecutor(executor);
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
     * @brief 获取当前线程的IOManager, 应该只从IOManager线程访问.
     * @return IOManager指针, 返回不拥有指针所有权, 指针有效期最短与thread local变量相同.
    */
    static std::shared_ptr<IOManager> getThreadLocal();

    /**
     * @brief 设置当前线程的IOManager, 应该只从IOManager线程访问.
    */
    static void setThreadLocal(std::shared_ptr<IOManager> io_manager);
private:
    struct FdEvents
    {
        uint32_t registered_events = 0;//fd 已注册事件类型
        bool read_call_once = true;
        bool write_call_once = true;
        coroutine::Executor::Ptr read_executor = nullptr;//fd对应的读事件执行器
        coroutine::Executor::Ptr write_executor = nullptr;//fd对应的写事件执行器
    };


    void initEpoll();

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
    coroutine::Scheduler scheduler_;
    TimerManager timer_manager_;
    std::vector<FdEvents> fd_events_;
};



} // namespace lon::io
