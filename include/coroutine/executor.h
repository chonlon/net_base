#pragma once
#include "base/macro.h"
#include "base/typedef.h"
#include "executor.h"

#include <cassert>
#include <functional>
#include <memory>
#include <ucontext.h>


namespace lon {

namespace coroutine {
namespace executor_info {
size_t idGenerate();
void releaseId(size_t id);
void createUpdateData() noexcept;
void destroyUpdateData() noexcept;
}  // namespace executor_info


void callerExecutorFunc();


/**
 * @brief 协程中的单个执行单元, 大致相当于进程中的process.
 * 和scheduler紧密耦合.
 */
class Executor : public std::enable_shared_from_this<Executor>
{
public:
    using ExectutorFunc = std::function<void()>;
    using Ptr           = std::shared_ptr<Executor>;


    //类似进程的五种状态, 调度器根据状态调度协程.
    enum class State : int16_t
    {
        Init,
        Exec,
        Ready,
        HoldUp,
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
        context_ = new struct ucontext_t;
        getCurrentContext();
        executor_info::createUpdateData();
    }

    /**
     * @brief Construct a new Executor object by callback.
     * 
     * @param _stack_size runing stack size.
     * @param _callback  runing callback, not null.
     */
    Executor(ExectutorFunc _callback, size_t _stack_size = 8 * data::K)
        : state_{State::Init},
          id_{executor_info::idGenerate()},
          stack_size_{_stack_size},
          stack_{nullptr},
          callback_{_callback} {
        assert(callback_ != nullptr);
        executor_info::createUpdateData();
    }


    Executor(const Executor& _other) = delete;
    Executor(Executor&& _other) noexcept = delete;
    auto operator=(const Executor& _other) -> Executor& = delete;
    auto operator=(Executor&& _other) noexcept -> Executor& = delete;

    ~Executor();

    // Init/HoldUp/Ready --> Exec
    void exec();

    // Exec --> Holdup
    void yield();


    // any --> aborted
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

    /**
     * @brief get current executor in this thread.
     * if thread does not have main executor(which means not initialized), this
     * func will turn current context into executor.
     * @return current executor ptr.
     */
    static Ptr getCurrent();

    /**
     * @brief init thread main executor
     * @return main executor ptr.
     */
    static Ptr initMainExecutor();

private:
    void terminal();

    // two executor in same thread.
    static void swapContext(Executor* dst, Executor* src);
    void initToReady();
    void makeContext();


    static size_t totalExectutors();
    static size_t getCurrentId();
    static size_t totalExecutorCount();

    static void executorMainFunc();

    void execInner();
    void yieldInner();
    void terminalInner();

    void getCurrentContext();

private:
    State state_;
    size_t id_;
    size_t stack_size_;
    void* stack_;  // 执行单元的栈.
    ExectutorFunc callback_;
    //ucontext 的size大约是1KB, 使用指针而不是直接作为成员, 这样的话处于init状态的executor可以很大程度上减少内存消耗.
    ucontext_t* context_{nullptr}; 
};


}  // namespace coroutine

}  // namespace lon
