#pragma once
#include "../base/macro.h"
#include "../base/singleton.h"
#include "../base/typedef.h"
#include <cstdlib>
#include <cstring>
#include <vector>

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
    _FdManager() {}

    ~_FdManager() {
        for(auto context : contexts_) {
            if(!!context) {
                delete context;
            }
        }
    }

    void setContext(int fd, FdContext context) {
        WriteLocker<RWMutex> lock(mutex_);
        const int len = static_cast<int>(contexts_.size());
        constexpr double resizeFactor = 1.5;
        if(fd >= len) {
            contexts_.resize(static_cast<size_t>(fd * resizeFactor), nullptr);
        }
        contexts_[fd] = new FdContext(context);
    }

    /**
     * @brief 获取fd context的指针, 如果没有初始化, 那么返回nullptr.
     */
    LON_NODISCARD
    FdContext* getContext(int fd) const {
        ReadLocker<RWMutex> locker(mutex_);
        const int len = static_cast<int>(contexts_.size());
        if (fd < len && !!contexts_[fd]) {
            return contexts_[fd];
        }
        return nullptr;
    }

    bool hasFd(int fd) {
        ReadLocker<RWMutex> locker(mutex_);
        const int len = static_cast<int>(contexts_.size());
        if (fd < len && !!contexts_[fd])
            return true;
        return false;
    }

    void delContext(int fd) {
        WriteLocker<RWMutex> lock(mutex_);
        const int len = static_cast<int>(contexts_.size());
        if (fd < len && !!contexts_[fd]) {
            delete contexts_[fd];
            contexts_[fd] = nullptr;
        }
    }

private:
    //如果确定socket在fd中不是稠密分布的, 那么使用unordered_map会更好.
    std::vector<FdContext*> contexts_;

    mutable RWMutex mutex_;
};

using FdManager = Singleton<_FdManager>;

}  // namespace lon::io
