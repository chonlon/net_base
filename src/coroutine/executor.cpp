#include "coroutine/executor.h"

#include "logger.h"

#include <atomic>
#include <fmt/format.h>


static auto G_logger = lon::LogManager::getInstance() -> getLogger("system");

namespace lon {
namespace coroutine {
constexpr int DefaultStackSize = 8 * data::K;

thread_local Executor::Ptr t_cur_executor{nullptr};
thread_local Executor::Ptr t_main_executor{nullptr};

std::atomic<size_t> least_unallocated_id{0};
std::atomic<size_t> executor_count{0};


//目前来说size_t就算超长时间运行是不会出现重复, 所以采用简单的方式分配id,
//同时可以节省很多内存. 不然的话, 可能需要考虑使用set来保存已分配内存,
//或者参考fd分配方式.

size_t executor_info::idGenerate() {
    return ++least_unallocated_id;
}

void executor_info::releaseId(size_t id) {
    // do nothing.
}

void executor_info::createUpdateData() noexcept {
    ++executor_count;
}

void executor_info::destroyUpdateData() noexcept {
    --executor_count;
}

void Executor::executorMainFunc() {
    try {
        assert(t_cur_executor);
        assert(t_cur_executor->callback_);
        t_cur_executor->callback_();
    } catch (...) {
        throw;
    }
    t_cur_executor->terminal();
}

Executor::~Executor() {
    if (this == t_main_executor.get())
        LON_LOG_ERROR(G_logger) << fmt::format(
            "destroying thread main executor! thread id:{}, thread name:{}",
            getThreadId(),
            getThreadName());
    executor_info::destroyUpdateData();
    executor_info::releaseId(id_);
    
    if (stack_)
        free(stack_);
}

void Executor::exec() {
    assert(this !=
           t_main_executor
               .get());  //当前逻辑exec是切换当main_executor执行,
                         //使用t_main_executor调用executor相当于没有调用.

    switch (state_) {
        case State::Init:
            initToReady();
            [[fallthrough]];
        case State::HoldUp:
        case State::Ready:
            state_ = State::Exec;
            execInner();
            break;
        case State::Exec:
            LON_LOG_DEBUG(G_logger)
                << getCurrentId() << " is executing, do nothing\n";
            break;
        case State::Terminal:
        case State::Aborted:
            LON_LOG_WARN(G_logger)
                << getCurrentId() << " has terminal or aborted, do nothing\n";
            break;
        default:
            LON_LOG_ERROR(G_logger) << "state may be not init!!!";
            break;
    }
}

void Executor::yield() {
    if (state_ != State::Exec)
        return;
    state_ = State::HoldUp;
    yieldInner();
}

void Executor::kill() {
    State cur_state = state_;
    state_          = State::Aborted;
    if (cur_state == State::Exec)
        terminalInner();
}

void Executor::terminal() {
    // cannot invoke external.
    // check state.
    assert(state_ == State::Exec);
    state_ = State::Terminal;
    terminalInner();
}


void Executor::reset(ExectutorFunc func, bool back_to_caller) {
    callback_ = func;
    makeContext();
}

void Executor::swapContext(Executor* dst, Executor* src) {
    if (swapcontext(&dst->context_, &src->context_)) {
        LON_LOG_FATAL(G_logger)
            << "swap context failed, executor id:" << src->id_ << ';'
            << dst->id_ << "stack:" << backtraceString();
    }
}

void Executor::initToReady() {
    makeContext();
    state_ = State::Ready;
}

void Executor::makeContext() {
    if (stack_size_ == 0)
        stack_size_ = DefaultStackSize;

    if (stack_)
        free(stack_);
    else
        stack_ = malloc(stack_size_);

    getCurrentContext();
    context_.uc_link          = nullptr;
    context_.uc_stack.ss_sp   = stack_;
    context_.uc_stack.ss_size = stack_size_;


    makecontext(&context_, &executorMainFunc, 0);
}

Executor::Ptr Executor::initMainExecutor() {
    Ptr thread_main_executor = std::make_shared<Executor>();
    t_main_executor          = thread_main_executor;
    t_cur_executor           = thread_main_executor;
    return thread_main_executor;
}

Executor::Ptr Executor::getCurrent() {
    if (t_cur_executor)
        return t_cur_executor;

    return initMainExecutor();
}

size_t Executor::getCurrentId() {
    return t_cur_executor ? t_cur_executor->id_ : 0;
}

size_t Executor::totalExecutorCount() {
    return executor_count;
}

void Executor::execInner() {
    t_cur_executor = this->shared_from_this();
    swapContext(t_main_executor.get(), this);
}

void Executor::yieldInner() {
    t_cur_executor = t_main_executor;
    swapContext(this, t_cur_executor.get());
}

void Executor::terminalInner() {
    t_cur_executor = t_main_executor;
    swapContext(this, t_cur_executor.get());
}

void Executor::getCurrentContext() {
    if (getcontext(&context_)) {
        LON_LOG_ERROR(G_logger)
                << "get context failed" << backtraceString();
    }
}

}  // namespace coroutine

}  // namespace lon
