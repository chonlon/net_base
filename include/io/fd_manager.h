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

    // bool setIsSocket (int fd, bool value) {
    // 	WriteLocker<RWMutex> wlocker(mutex_);
    // 	if(fd > len_) return false;
    // 	context_[fd].is_socket = value;
    // 	return true;
    // }
    // bool setIsInitialized (int fd, bool value) {
    // 	WriteLocker<RWMutex> wlocker(mutex_);
    // 	if(fd > len_) return false;
    // 	context_[fd].is_initialized = value;
    // 	return true;
    // }
    // bool setIsClosed (int fd, bool value) {
    // 	WriteLocker<RWMutex> wlocker(mutex_);
    // 	if(fd > len_) return false;
    // 	context_[fd].is_closed = value;
    // 	return true;
    // }
    // bool setIsUserNonBlock (int fd, bool value) {
    // 	WriteLocker<RWMutex> wlocker(mutex_);
    // 	if(fd > len_) return false;
    // 	context_[fd].is_user_non_block = value;
    // 	return true;
    // }
    // bool setIsSysNonBlock (int fd, bool value) {
    // 	WriteLocker<RWMutex> wlocker(mutex_);
    // 	if(fd > len_) return false;
    // 	context_[fd].is_sys_non_block = value;
    // 	return true;
    // }
    //
    // std::pair<bool, bool> getIssocket(int fd) {
    // 	ReadLocker<RWMutex> locker(mutex_);
    // 	if(fd > len_) return {false, false};
    // 	return {context_[fd].is_socket, true};
    // }
    // std::pair<bool, bool> getIsinitialized(int fd) {
    // 	ReadLocker<RWMutex> locker(mutex_);
    // 	if(fd > len_) return {false, false};
    // 	return {context_[fd].is_initialized, true};
    // }
    // std::pair<bool, bool> getIsclosed(int fd) {
    // 	ReadLocker<RWMutex> locker(mutex_);
    // 	if(fd > len_) return {false, false};
    // 	return {context_[fd].is_closed, true};
    // }
    // std::pair<bool, bool> getIsUserNonBlock(int fd) {
    // 	ReadLocker<RWMutex> locker(mutex_);
    // 	if(fd > len_) return {false, false};
    // 	return {context_[fd].is_user_non_block, true};
    // }
    // std::pair<bool, bool> getIsSysNonBlock(int fd) {
    // 	ReadLocker<RWMutex> locker(mutex_);
    // 	if(fd > len_) return {false, false};
    // 	return {context_[fd].is_sys_non_block, true};
    // }

private:
    void allocToNewSize() {
        constexpr double factor = 1.5;

        FdContext* dst = new FdContext[static_cast<int>(len_ * factor)]; 
        std::memcpy(dst, context_, len_ * sizeof(FdContext));
        // free(context_);
        delete dst;
        context_ = dst;
        len_     = static_cast<int>(len_ * factor);
    }

    FdContext* context_;
    int len_ = 1;
    mutable RWMutex mutex_;
};

using FdManager = Singleton<_FdManager>;

}  // namespace lon::io
