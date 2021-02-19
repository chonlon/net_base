#pragma once

#if __cplusplus >= 201703L
#define LON_HAS_CPP17
#endif

#ifdef LON_HAS_CPP17
#define LON_NODISCARD [[nodiscard]]
#else
#define LON_NODISCARD
#endif

#ifdef __linux__
    #define LON_LINUX
#elif _WIN32
    #define LON_WINDOWS
#else
#define LON_OTHER_OS
#endif

#define LON_HAVE_O_CLOEXEC 1


#define LON_ERROR_INVOKE_ASSERT(condition, func_name, logger) \
    	if (!(condition)) { \
            LON_LOG_ERROR(logger) << fmt::format(#func_name " failed, with error: {}", std::strerror(errno)); \
            assert(false); \
        }
