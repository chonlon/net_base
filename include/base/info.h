#pragma once

#include "typedef.h"
#include "macro.h"
#include "string_piece.h"

#include <sys/time.h>

#include <unistd.h>
#include <sys/syscall.h>


namespace lon {
    extern thread_local uint32_t G_ThreadId;
    extern thread_local String G_ThreadName;

    /**
     * @brief raw api获取线程ID
    */
    inline uint32_t getThreadIdRaw() noexcept {
        return static_cast<uint32_t>(::syscall(SYS_gettid));
    }

    /**
     * @brief Get the Thread id, 使用thread_local来缓存数据, 所以在线程退出环境中可能是错误的.
     * 
     * @return uint32_t 线程id.
     */
    inline uint32_t getThreadId() noexcept {
        if(UNLIKELY(G_ThreadId == 0)) { //thread id is not cached.
            G_ThreadId = getThreadIdRaw();
        }
        return G_ThreadId;
    }

    /**
     * @brief raw api 获取线程名.
    */
    inline String getThreadNameRaw() {
        char buf[1024];
        pthread_getname_np(pthread_self(), buf, 1024);
        return String(buf);
    }

    size_t getExecutorId();

    /**
     * @brief 获取线程名字, 使用thread_local来缓存数据, 所以在线程退出环境中可能是错误的.
     * @return 
    */
    inline StringPiece getThreadName() noexcept {
        return G_ThreadName;
    }

    /**
     * @brief 设置线程名字
    */
    inline void setThreadName(StringArg name) noexcept {
        G_ThreadName = name.str;
        pthread_setname_np(pthread_self(), G_ThreadName.substr(0,15).c_str());
    }

    /**
     * \brief 获得当前主机名
     * \return 主机名
     * \exception lon::ExecFailed 如果执行5次获取主机名函数都失败
     */
    StringPiece getHostName();

    String getHostWithoutBuffer();

    String backtraceString(int depth = 10, int skip = 1);

    inline pid_t getPid() {
        return ::getpid();
    }

    inline pid_t getUid() {
        return ::getuid();
    }

    inline size_t currentMs() {
        struct timeval _timerval;
        gettimeofday(&_timerval,nullptr);
        return _timerval.tv_sec * 1000 + _timerval.tv_usec / 1000;
    }
}
