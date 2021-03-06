#pragma once
#include "../base/macro.h"
#include "../base/typedef.h"
#include "../cmake_defination.h"

#include <cassert>
#include <functional>
#include <memory>
#if LON_CONTEXT_TYPE == COROUTINE_UCONTEXT
#include <ucontext.h>
#elif  LON_CONTEXT_TYPE == COROUTINE_FCONTEXT
#include <boost/context/detail/fcontext.hpp>
#endif

namespace lon {

namespace coroutine {
    // ucontext_t缺点:  ucontext_t的上下文切换比fcontext_t多占用两个数量级的CPU周期;
    // ucontext_t原本设计用于多线程切换的, 所以保存了信号掩码, 而协程其实不用.

#if LON_CONTEXT_TYPE == COROUTINE_UCONTEXT
    using ContextType = ucontext_t *;
#elif  LON_CONTEXT_TYPE == COROUTINE_FCONTEXT
    using ContextType = boost::context::detail::fcontext_t;
#endif

namespace executor_info {
size_t idGenerate();
void releaseId(size_t id);
void createUpdateData() noexcept;
void destroyUpdateData() noexcept;
}  // namespace executor_info

constexpr int DefaultStackSize = 64 * data::K;

void callerExecutorFunc();


/**
 * @brief 协程中的单个执行单元, 大致相当于进程中的process.
 * 和scheduler紧密耦合.
 */
class Executor : public std::enable_shared_from_this<Executor>
{
    friend class Scheduler;

public:
    using ExectutorFunc = std::function<void()>;
    using Ptr           = std::shared_ptr<Executor>;


    //类似进程的五种状态, 调度器根据状态调度协程.
    enum class State : int16_t
    {
        Init,
        Exec,
        Ready,
        HoldUp,  //挂起, 空闲时被唤醒.
        Terminal,
        Aborted  // 异常终止
    };

    /**
     * @brief Construct a new Executor object by current context.
     *
     */
    Executor()
        : state_{State::Ready},
          id_{executor_info::idGenerate()},
          stack_size_{0},
          stack_{nullptr} {
        newContext();
        getCurrentContext();
        executor_info::createUpdateData();
    }

    /**
     * @brief Construct a new Executor object by callback.
     *
     * @param _stack_size running stack size.
     * @param _callback  running callback, not null.
     */
    Executor(ExectutorFunc&& _callback, size_t _stack_size = DefaultStackSize)
        : is_call_back_type_{true},
          state_{State::Init},
          id_{executor_info::idGenerate()},
          stack_size_{_stack_size},
          stack_{nullptr},
          callback_{std::move(_callback)} {
        assert(callback_ != nullptr);
        executor_info::createUpdateData();
    }


    Executor(const Executor& _other)     = delete;
    Executor(Executor&& _other) noexcept = delete;
    auto operator=(const Executor& _other) -> Executor& = delete;
    auto operator=(Executor&& _other) noexcept -> Executor& = delete;

    ~Executor();

    // Init/HoldUp/Ready --> Exec
    void exec();

    // Exec --> Holdup
    void yield();

    // // Exec/Holdup--> Blocking
    // void block();
    //
    // // Blocking
    // void unblock();

    // any --> terminal
    void kill();


    LON_NODISCARD auto getState() const -> State {
        return state_;
    }

    LON_NODISCARD auto getId() const -> uint64_t {
        return id_;
    }


    auto setId(uint64_t _id) -> void {
        id_ = _id;
    }

    void reset(ExectutorFunc func, bool back_to_caller);

    bool isCallbackType() {return is_call_back_type_;}
    void reuse() {
        if(is_call_back_type_ && state_ == State::Terminal) {
            state_ = State::Ready;
            resetContext();
        }
    }

    /**
     * @brief get current executor in this thread.
     * if thread does not have main executor(which means not initialized), this
     * func will turn current context into executor.
     * @return current executor ptr.
     */
    static Ptr getCurrent();


    static void setMainExecutor(Executor::Ptr executor);

private:
    void mainExec();
    void mainYield();
    void mainExecInner();
    void mainYieldInner();

    void terminal();

    void doExec(bool main);

    // two executor in same thread.
    static void swapContext(Executor* dst, Executor* src);
    void initToReady();
    void newContext();
    void makeContext();
    void getCurrentContext();
    void resetContext();

    static size_t totalExectutors();
    static size_t getCurrentId();
    static size_t totalExecutorCount();
#if LON_CONTEXT_TYPE == COROUTINE_UCONTEXT
    static void executorMainFunc();
#elif  LON_CONTEXT_TYPE == COROUTINE_FCONTEXT
    void fixStackUnwinding();
    static void executorMainFunc(boost::context::detail::transfer_t transfer);
#endif
    

    void execInner();
    void yieldInner();
    void terminalInner();
private:
    bool is_call_back_type_ = false;
    State state_;
    size_t id_;
    size_t stack_size_;
    unsigned char* stack_;  // 执行单元的栈.
    ExectutorFunc callback_;
    // ucontext 的size大约是1KB, 使用指针而不是直接作为成员,
    // 这样的话处于init状态的executor可以很大程度上减少内存消耗.
    ContextType context_{nullptr};
};


}  // namespace coroutine

}  // namespace lon
