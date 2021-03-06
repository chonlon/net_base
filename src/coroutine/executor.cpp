#include "coroutine/executor.h"

#include "logger.h"

#include <atomic>
#include <fmt/format.h>


static auto G_logger = lon::LogManager::getInstance() -> getLogger("system");

namespace lon {
namespace coroutine {


thread_local Executor::Ptr t_cur_executor{nullptr};
thread_local Executor::Ptr t_main_executor{nullptr};
thread_local Executor::Ptr t_base_executor{nullptr};

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
#if LON_CONTEXT_TYPE == COROUTINE_UCONTEXT
void Executor::executorMainFunc() {
#elif  LON_CONTEXT_TYPE == COROUTINE_FCONTEXT

void Executor::fixStackUnwinding() {
    if (kIsArchAmd64 && kIsLinux) {
        // Extract RBP and RIP from main context to stitch main context stack and
        // fiber stack.
        auto stackBase = reinterpret_cast<void**>(stack_ + stack_size_);
        auto mainContext = reinterpret_cast<void**>(context_);
        stackBase[-2] = mainContext[6];
        stackBase[-1] = mainContext[7];
    }
}

void Executor::executorMainFunc(boost::context::detail::transfer_t transfer) {
    t_cur_executor->fixStackUnwinding();
    //TODO 有把src的指针以data传入再赋值更好的实现方式吗
    // 设置swapcontext的src的fcontext.
    static_cast<Executor*>(transfer.data)->context_ = transfer.fctx;
    
#endif
    try {
        assert(t_cur_executor);
        assert(t_cur_executor->callback_);
        t_cur_executor->callback_();
    } catch (std::exception& e) {
        std::exception_ptr curr_exp = std::current_exception();
        LON_LOG_ERROR(G_logger) << fmt::format("{} got an exception, what:{}, bt:\n{}", t_cur_executor->getId(), e.what(), backtraceString());
    }
    catch (...) {
        LON_LOG_ERROR(G_logger) << fmt::format("{} got an exception, bt:\n{}", t_cur_executor->getId(), backtraceString());
    }
    
    t_cur_executor->terminal();
}


Executor::~Executor() {
    if (UNLIKELY(this == t_main_executor.get()))
        // 不能使用logger, 因为此时主协程正在析构.
        fmt::print(
            "notion: destroying thread main executor! thread id:{}, thread "
            "name:{}\n",
            getThreadIdRaw(),
            getThreadNameRaw());
    executor_info::destroyUpdateData();
    executor_info::releaseId(id_);

    if (stack_)
        free(stack_);
#if LON_CONTEXT_TYPE == COROUTINE_UCONTEXT
    if(context_)
        delete context_;
#endif
}

void Executor::exec() {
    assert(this != t_main_executor.get());  //当前逻辑exec是切换当main_executor执行,
                                            //使用t_main_executor调用executor相当于没有调用.

    doExec(false);

}

void Executor::yield() {
    if (state_ != State::Exec)
        return;
    state_ = State::HoldUp;
    yieldInner();
}

// void Executor::block() {
//     if(state_ == State::Exec) {
//         state_ = State::Blocking;
//         yieldInner();
//     } else if(state_ == State::HoldUp) {
//         state_ = State::Blocking;
//     } else {
//         return;
//     }
// }
//
// void Executor::unblock() {
//     if (state_ == State::Blocking) {
//         state_ = State::HoldUp;
//     }
// }

void Executor::kill() {
    State cur_state = state_;
    state_          = State::Terminal;
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
    if(stack_)
        resetContext();
    else 
        makeContext();
}

void Executor::swapContext(Executor* src, Executor* dst) {
#if LON_CONTEXT_TYPE == COROUTINE_UCONTEXT
    if (UNLIKELY(swapcontext(src->context_, dst->context_))) {
        LON_LOG_FATAL(G_logger)
            << "swap context failed, executor id:" << src->id_ << ';'
            << dst->id_ << "stack:\n" << backtraceString();
    }
#elif  LON_CONTEXT_TYPE == COROUTINE_FCONTEXT
    auto transfer = boost::context::detail::jump_fcontext(dst->context_, src);
    dst->context_ = transfer.fctx;
#endif

}

void Executor::initToReady() {
    makeContext();
    state_ = State::Ready;
}

void Executor::newContext() {
#if LON_CONTEXT_TYPE == COROUTINE_UCONTEXT
    context_ = new struct ucontext_t;
#elif  LON_CONTEXT_TYPE == COROUTINE_FCONTEXT
#endif
}

void Executor::makeContext() {
    if (UNLIKELY(stack_size_ == 0))
        stack_size_ = DefaultStackSize;

    if (stack_)
        free(stack_);
    else
        stack_ = static_cast<unsigned char*>(malloc(stack_size_));
    newContext();
    getCurrentContext();
#if LON_CONTEXT_TYPE == COROUTINE_UCONTEXT
    context_->uc_link = nullptr;
    context_->uc_stack.ss_sp = stack_;
    context_->uc_stack.ss_size = stack_size_;

    makecontext(context_, &executorMainFunc, 0);
#elif  LON_CONTEXT_TYPE == COROUTINE_FCONTEXT
   auto stack_base = stack_ + stack_size_;
    context_ =
        boost::context::detail::make_fcontext(stack_base, stack_size_, &executorMainFunc);
#endif

}

void Executor::getCurrentContext() {
#if LON_CONTEXT_TYPE == COROUTINE_UCONTEXT
    if (UNLIKELY(getcontext(context_))) {
        LON_LOG_ERROR(G_logger) << "get context failed" << backtraceString();
    }
#elif  LON_CONTEXT_TYPE == COROUTINE_FCONTEXT
#endif

}

void Executor::resetContext() {
    bzero(stack_, stack_size_);
#if LON_CONTEXT_TYPE == COROUTINE_UCONTEXT
    context_->uc_link = nullptr;
    context_->uc_stack.ss_sp = stack_;
    context_->uc_stack.ss_size = stack_size_;

    makecontext(context_, &executorMainFunc, 0);
#elif  LON_CONTEXT_TYPE == COROUTINE_FCONTEXT
    auto stack_base = stack_ + stack_size_;
    boost::context::detail::make_fcontext(stack_base, stack_size_, &executorMainFunc);
#endif
}

void Executor::setMainExecutor(Executor::Ptr executor) {
    t_main_executor = executor;
}

void Executor::mainExec() {
    doExec(true);
}

void Executor::mainYield() {
    if (state_ != State::Exec)
        return;
    state_ = State::HoldUp;
    mainYieldInner();
}
void Executor::mainExecInner() {
    t_cur_executor = this->shared_from_this();
    swapContext(t_base_executor.get(), this);
}

void Executor::mainYieldInner() {
    t_cur_executor = t_base_executor;
#if LON_CONTEXT_TYPE == COROUTINE_FCONTEXT
    fixStackUnwinding();
#endif
    swapContext(this, t_base_executor.get());
}
void Executor::doExec(bool main) {
    switch (state_) {
        case State::Init:
            initToReady();
            [[fallthrough]];
        case State::HoldUp:
        case State::Ready:
            state_ = State::Exec;
            if(UNLIKELY(main))
                mainExecInner();
            else
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

Executor::Ptr Executor::getCurrent() {
    if (LIKELY(!!t_cur_executor))
        return t_cur_executor;
    
    t_cur_executor = std::make_shared<Executor>();
    t_base_executor = t_cur_executor;
    if(!t_main_executor) setMainExecutor(t_cur_executor);

    return t_cur_executor;
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
#if LON_CONTEXT_TYPE == COROUTINE_FCONTEXT
    fixStackUnwinding();
#endif
    swapContext(this, t_cur_executor.get());
}

void Executor::terminalInner() {
    t_cur_executor = t_main_executor;
    swapContext(this, t_cur_executor.get());
}



}  // namespace coroutine

}  // namespace lon
