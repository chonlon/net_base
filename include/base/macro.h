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

//always inline
#if defined(__GNUC__)
#define LON_ALWAYS_INLINE inline __attribute__((__always_inline__))
#else
#define LON_ALWAYS_INLINE inline
#endif


// likely
#undef LIKELY
#undef UNLIKELY

#if defined(__GNUC__)
#define LIKELY(x) (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif



#define LON_HAVE_O_CLOEXEC 1


#define LON_ERROR_INVOKE_ASSERT(condition, func_name, info, logger) \
    	if (UNLIKELY(!(condition))) { \
            LON_LOG_ERROR(logger) << fmt::format(#func_name " failed, with error: {} info: {}", std::strerror(errno), info); \
            assert(false); \
        }
