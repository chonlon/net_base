#include "base/info.h"
#include "base/expection.h"

#include <unistd.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <cstring>
#include <fmt/core.h>
#include "coroutine/executor.h"

namespace lon {
thread_local uint32_t G_ThreadId = 0;
thread_local String G_ThreadName = "UnSet";

size_t getExecutorId() {
    return coroutine::Executor::getCurrent()->getId();
}

StringPiece getHostName() {
    static String host_name = getHostWithoutBuffer(); // 使用static可以比起全局变量可以控制初始化顺序
    return StringPiece(host_name);
}

String getHostWithoutBuffer() {
    char hostname[1024]{};
    int i = 0;
    for (; i < 5; ++i) {
        if ((::gethostname(hostname, 1024)) == 0)
            break;
    }
    if (UNLIKELY(i == 5))
        throw ExecFailed(
            fmt::format("get hostname failed with errno:{}", errno));
    return String(static_cast<const char*>(hostname));
}

bool backtraceStacks(std::vector<String>& stacks, int depth, int skip) {
    std::unique_ptr<void*> frame(
        static_cast<void**>(::malloc(sizeof(void*) * depth)));
    int s          = ::backtrace(frame.get(), depth);
    char** strings = ::backtrace_symbols(frame.get(), s);
    if (strings) {
        for (int i = skip; i < s; ++i) {
            stacks.push_back(strings[i]);
        }
        free(strings);
        return true;
    }
    return false;
}

String backtraceString(int depth, int skip) {
    std::unique_ptr<void*> frame(
        static_cast<void**>(::malloc(sizeof(void*) * depth)));
    const int count      = ::backtrace(frame.get(), depth);
    char** strings = ::backtrace_symbols(frame.get(), count);

    std::string stack_string;

    if (strings) {
        size_t len      = 256;
        char* demanding_buffer = static_cast<char*>(::malloc(len));

        for (int i = skip; i < count; ++i) {
            // fmt::print("{}\n", strings[i]);

            char* left_par = nullptr;
            char* plus     = nullptr;
            for (char* p = strings[i]; *p; ++p) {
                if (*p == '(')
                    left_par = p;
                else if (*p == '+')
                    plus = p;
            }

            if (left_par && plus) {
                *plus      = '\0';
                int status = 0;
                // convert name to readable (--like c++filt)
                char* ret  = abi::__cxa_demangle(
                    left_par + 1,
                    demanding_buffer,
                    &len,
                    &status);
                *plus = '+';
                if (status == 0) {
                    demanding_buffer = ret; // ret could be realloc()
                    stack_string.append(strings[i], left_par + 1);
                    stack_string.append(demanding_buffer);
                    stack_string.append(plus);
                    stack_string.push_back('\n');
                    continue;
                }
            }

            stack_string.append(strings[i]);
            stack_string.push_back('\n');
        }

    } else {
        //TODO log to error;
    }
    return stack_string;
}

}
