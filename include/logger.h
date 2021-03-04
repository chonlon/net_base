#pragma once

#include "base/info.h"
#include "base/string_piece.h"
#include "base/typedef.h"
#include "logging/logger_flusher.h"
#include <algorithm>
#include <array>
#include <assert.h>
#include <functional>
#include <memory>
#include <sstream>
#include <string>


#define LON_LOG(logger, level)                                        \
    if (logger->getLevel() <= level)                                  \
    lon::LogWrapper(logger,                                           \
                    lon::LogEvent(__FILE__,                           \
                                  __LINE__,                           \
                                  level,                              \
                                  lon::getThreadId(),                 \
                                  0,                                  \
                                  lon::getExecutorId(),               \
                                  time(0) /*,lon::getThreadName()*/)) \
        .stream

#define LON_LOG_DEBUG(logger) LON_LOG(logger, lon::Level::DEBUG)
#define LON_LOG_INFO(logger) LON_LOG(logger, lon::Level::INFO)
#define LON_LOG_WARN(logger) LON_LOG(logger, lon::Level::WARN)
#define LON_LOG_ERROR(logger) LON_LOG(logger, lon::Level::ERROR)
#define LON_LOG_FATAL(logger) LON_LOG(logger, lon::Level::FATAL)

#define LON_LOG_DEFAULT_DEBUG() \
    LON_LOG_DEBUG(LogManager::getInstance()->getDefault())
#define LON_LOG_DEFAULT_INFO() \
    LON_LOG_INFO(LogManager::getInstance()->getDefault())
#define LON_LOG_DEFAULT_WARN() \
    LON_LOG_WARN(LogManager::getInstance()->getDefault())
#define LON_LOG_DEFAULT_ERROR() \
    LON_LOG_ERROR(LogManager::getInstance()->getDefault())
#define LON_LOG_DEFAULT_FATAL() \
    LON_LOG_FATAL(LogManager::getInstance()->getDefault())

namespace lon {
constexpr int flusher_max = 10;
class LogFlusher;
class Logger;
class _LogManager;

enum Level
{
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    SIZE
};

struct LogEvent
{
    using StringStream = std::stringstream;


    const char* file_name;  // using string_view to save size?
    int line;
    Level level;
    uint32_t thread_id;
    uint32_t elapsed_ms;
    size_t executor_id;
    // StringPiece thread_name;
    StringPiece logger_name{};       // set in logger //耦合Logger
    StringPiece datetime_pattern{};  // set in logger //耦合Logger
    String content{};  // set in LoggerWrapper //耦合LoggerWrapper
    time_t time;


    LogEvent(const LogEvent& _other)     = default;
    LogEvent(LogEvent&& _other) noexcept = default;
    auto operator=(const LogEvent& _other) -> LogEvent& = default;
    auto operator=(LogEvent&& _other) noexcept -> LogEvent& = default;
    ~LogEvent() = default;

    LogEvent(const char* _file_name,
             int _line,
             Level _level,
             uint32_t _thread_id,
             uint32_t _elapsed_ms,
             size_t _executor_id,
             time_t _time)
        : file_name{_file_name},
          line{_line},
          level{_level},
          thread_id{_thread_id},
          elapsed_ms{_elapsed_ms},
          executor_id{_executor_id},
          time{_time} {}
};

struct LogWrapper
{
public:
    using StringStream = std::stringstream;
    StringStream stream;
    std::shared_ptr<Logger> logger_ptr;
    LogEvent event;


    LogWrapper(std::shared_ptr<Logger> _logger_ptr, LogEvent _event);

    ~LogWrapper();
};


class Logger : public std::enable_shared_from_this<Logger>
{
    friend _LogManager;

public:
    using StringStream  = std::stringstream;
    using ptr           = std::shared_ptr<Logger>;
    using FormatterFunc = std::function<void(StringStream& stream, LogEvent*)>;
    // noexcept

    Logger(const String& name) : name_{name} {}

    inline static const char* levelToString(Level level) noexcept {
        static const char* LogLevelName[Level::SIZE] = {
            "DEBUG",
            "INFO",
            "WARN",
            "ERROR",
            "FATAL",
        };
        assert(level >= Level::DEBUG);
        assert(level < Level::SIZE);
        return LogLevelName[level];
    }

    static Level levelFromString(StringPiece str) noexcept;

    Level getLevel() const noexcept {
        return level_;
    }


    void log(LogEvent*) noexcept;

    void addOneFlusher(std::unique_ptr<log::Flusher> flusher) {
        if (LIKELY(flusher != nullptr))
            flushers_[flusher_count_++] = std::move(flusher);
    }

    void setLevel(Level level) {
        level_ = level;
    }

    /**
     * @brief Set the Formatters object, if formatters is not set, log will be
     * empty
     *
     * @param formatter_pattern
     */
    void setFormatters(const String& formatter_pattern);

private:
    void registerUpdateFlusher() const;


    void addOneFormatter(FormatterFunc formatter) {
        formatters_.emplace_back(std::move(formatter));
    }


private:
    Level level_       = DEBUG;
    int flusher_count_ = 0;
    std::string name_{};
    std::string datetime_pattern_{};
    std::array<std::unique_ptr<log::Flusher>, flusher_max> flushers_{nullptr};
    std::vector<FormatterFunc> formatters_{};
};


class _LogManager : lon::Noncopyable
{
public:
    _LogManager();

    Logger::ptr getLogger(const String& key) {
        if (auto iter = loggers_.find(key); iter != loggers_.end())
            return iter->second;
        return nullptr;
    }

    Logger::ptr getDefault() {
        return default_logger_;
    }

private:
    void initDefaultLogger();
    void initLoggerFromConfig();

    Logger::ptr default_logger_ = nullptr;
    std::unordered_map<String, Logger::ptr> loggers_{};
};


using LogManager = Singleton<_LogManager>;


}  // namespace lon
