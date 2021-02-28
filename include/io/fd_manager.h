#pragma once
#include "base/macro.h"
#include "base/singleton.h"
#include "base/typedef.h"
#include <cstdlib>
#include <cstring>
namespace lon::io {

struct FdContext
{
    bool is_socket         = false;
    bool is_initialized    = false;
    bool is_user_non_block = false;
    bool is_sys_non_block  = false;

    size_t readTimeout  = -1;
    size_t writeTimeout = -1;


    FdContext(bool _is_socket) : is_socket{_is_socket}, is_initialized{true} {}

    FdContext() = default;
};

class _FdManager
{
public:
    _FdManager() : context_{nullptr}, len_{0} {
        constexpr int default_size = 10;
        context_ = new FdContext[default_size];
        len_ = default_size;
    }

    ~_FdManager() {
        delete[] context_;
    }

    void setContext(int fd, FdContext context) {
        WriteLocker<RWMutex> lock(mutex_);
        if (fd >= len_)
            allocToNewSize();
        context_[fd] = context;
    }

    /**
     * @brief 获取fd context的指针, 如果没有初始化, 那么返回nullptr.
     */
    LON_NODISCARD
    FdContext* getContext(int fd) const {
        ReadLocker<RWMutex> locker(mutex_);
        if (fd < len_ && context_[fd].is_initialized) {
            return &context_[fd];
        }
        return nullptr;
    }

    bool hasFd(int fd) {
        ReadLocker<RWMutex> locker(mutex_);
        if (fd < len_ && context_[fd].is_initialized)
            return true;
        return false;
    }

    void delContext(int fd) {
        WriteLocker<RWMutex> lock(mutex_);
        if (fd < len_ && context_[fd].is_initialized) {
            context_[fd].is_initialized = false;
        }
    }

private:
    void allocToNewSize() {
        constexpr double factor = 1.5;

        FdContext* dst = new FdContext[static_cast<int>(len_ * factor)];
        std::memcpy(dst, context_, len_ * sizeof(FdContext));
        delete[] dst;
        context_ = dst;
        len_     = static_cast<int>(len_ * factor);
    }

    FdContext* context_;
    int len_;
    mutable RWMutex mutex_;
};

using FdManager = Singleton<_FdManager>;

}  // namespace lon::io
