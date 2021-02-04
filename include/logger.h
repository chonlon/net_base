#pragma once

#include "base/string_piece.h"
#include "base/typedef.h"
#include "base/info.h"
#include "logging/logger_flusher.h"
#include <string>
#include <sstream>
#include <functional>
#include <algorithm>
#include <assert.h>
#include <memory>
#include <array>


#define LON_LOG(logger, level) \
    if(logger->getLevel() <= level) \
        lon::LogWrapper(logger, lon::LogEvent( __FILE__, __LINE__, level, lon::getThreadId(), \
                0, lon::getFiberId(), time(0)/*,lon::getThreadName()*/)).stream

#define LON_LOG_DEBUG(logger) LON_LOG(logger, lon::Level::DEBUG)
#define LON_LOG_INFO(logger) LON_LOG(logger, lon::Level::INFO)
#define LON_LOG_WARN(logger) LON_LOG(logger, lon::Level::WARN)
#define LON_LOG_ERROR(logger) LON_LOG(logger, lon::Level::ERROR)
#define LON_LOG_FATAL(logger) LON_LOG(logger, lon::Level::FATAL)

namespace lon {
constexpr int flusher_max   = 10;
class LogFlusher;
class Logger;
namespace detail {
    void initDefaultLogger();
    void initLoggerFromConfig();
}

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


    const char* file_name; //using string_view to save size?
    int line;
    Level level;
    uint32_t thread_id;
    uint32_t elapsed_ms;
    uint32_t fiber_id;
    // StringPiece thread_name;
    StringPiece logger_name{}; // set in logger //耦合Logger
    StringPiece datetime_pattern{};// set in logger //耦合Logger
    String content{};//set in LoggerWrapper //耦合LoggerWrapper
    time_t time;


    LogEvent(const LogEvent& _other)                        = default;
    LogEvent(LogEvent&& _other) noexcept                    = default;
    auto operator=(const LogEvent& _other) -> LogEvent&     = default;
    auto operator=(LogEvent&& _other) noexcept -> LogEvent& = default;

    LogEvent(const char* _file_name,
             int _line,
             Level _level,
             uint32_t _thread_id,
             uint32_t _elapsed_ms,
             uint32_t _fiber_id,
             time_t _time)
        : file_name{_file_name},
          line{_line},
          level{_level},
          thread_id{_thread_id},
          elapsed_ms{_elapsed_ms},
          fiber_id{_fiber_id},
          time{_time} {
    }
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
    friend void detail::initDefaultLogger();
    friend void detail::initLoggerFromConfig();
public:
    using StringStream = std::stringstream;
    using ptr = std::shared_ptr<Logger>;
    using FormatterFunc = std::function<void(StringStream& stream, LogEvent*)>;
    //noexcept

    Logger() {
        name_ = "root";
        setFormatters("%F%T[%p]%T[%c]%T<%f:%l>%T%m%n");
        // setFormatters("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T<%f:%l>%T%m%n");
        // SimpleStdoutFlusher* flusher = new SimpleStdoutFlusher;
        // addOneFlusher(flusher);
        SimpleFileFlusher* flusher2 = new SimpleFileFlusher{"/tmp/log1.txt"};
        addOneFlusher(flusher2);
        ProtectedFileFlusher* flusher3 = new ProtectedFileFlusher{"/tmp/log2.txt"};
        addOneFlusher(flusher3);
        AsyncFileLogFlusher* flusher4 = new AsyncFileLogFlusher{ "/tmp/log3.txt" };
        addOneFlusher(flusher4);
    }

    Logger(const String& name) : name_{name} {
        
    }

    inline static const char* levelToString(Level level) noexcept {
        static const char* LogLevelName[Level::SIZE] =
        {
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


    void log(LogEvent) noexcept;

private:
    void registerUpdateFlusher() const;
    void setFormatters(const String& formatter_pattern);

    void addOneFormatter(FormatterFunc formatter) {
        formatters_.emplace_back(std::move(formatter));
    }

    void addOneFlusher(LogFlusher* flusher) {
        flushers_[flusher_count_++].reset(flusher);
    }
private:
    Level level_         = DEBUG;
    int flusher_count_   = 0;
    std::string name_{};
    std::string datetime_pattern_{};
    std::array<std::unique_ptr<LogFlusher>, flusher_max> flushers_{nullptr};
    std::vector<FormatterFunc> formatters_{};
};



class LogFactory : lon::Noncopyable
{
    friend void detail::initDefaultLogger();
    friend void detail::initLoggerFromConfig();
public:
    static Logger::ptr getLogger(const String& key) {
        if(auto iter = loggers_.find(key); iter!=loggers_.end()) return iter->second;
        return nullptr;
    }

    static Logger::ptr getDefault() {
        return default_logger_;
    }
private:
    static Logger::ptr default_logger_;
    static std::unordered_map<String, Logger::ptr> loggers_;
};





} // namespace lon
