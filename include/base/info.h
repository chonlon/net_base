#pragma once

#include "typedef.h"
#include "macro.h"
#include "string_piece.h"

#include <unistd.h>
#include <sys/syscall.h>


namespace lon {
    extern thread_local uint32_t G_ThreadId;
    extern thread_local String G_ThreadName;

    inline uint32_t getThreadId() noexcept {
        if(G_ThreadId == 0) { //thread id is not cached.
            G_ThreadId = static_cast<int>(::syscall(SYS_gettid));
        }
        return G_ThreadId;
    }

    inline uint32_t getFiberId() noexcept {return 0;}

    inline StringPiece getThreadName() noexcept {
        return G_ThreadName;
    }

    inline void setThreadName(StringArg name) noexcept {
        G_ThreadName = name.str;
        pthread_setname_np(pthread_self(), name.str);
    }

    /**
     * \brief 获得当前主机名
     * \return 主机名
     * \exception lon::ExecFailed 如果执行5次获取主机名函数都失败
     */
    StringPiece getHostName();

    String backtraceString(int depth = 10, int skip = 1);
}
