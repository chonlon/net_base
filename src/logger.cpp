#include "logger.h"
#include "base/info.h"
#include "base/string_parsers.h"
#include "logging/loger_formatters.h"
#include "logging/logger_flusher.h"


#include "base.h"
#include "logging/logger_data_convert.h"
#include <fmt/core.h>
#include <iostream>

namespace lon {


Level Logger::levelFromString(StringPiece str) noexcept {
    std::string str_upper;
    std::transform(
        str.begin(),
        str.end(),
        std::back_inserter(str_upper),
        ::toupper);

#define LON_XX(name)          \
    if (str_upper == #name) { \
        return Level::name;   \
    }

    LON_XX(DEBUG)
    LON_XX(INFO)
    LON_XX(WARN)
    LON_XX(ERROR)
    LON_XX(FATAL)
#undef LON_XX

    return Level::SIZE;
}


LogWrapper::LogWrapper(std::shared_ptr<Logger> _logger_ptr, LogEvent _event)
    : stream{},
      logger_ptr{_logger_ptr},
      event{std::move(_event)} {
}

LogWrapper::~LogWrapper() {
    event.content = stream.str();
    Level level   = event.level;
    logger_ptr->log(std::move(event));
    if (level == Level::FATAL) {
        abort();
    }
}

void Logger::log(LogEvent event) noexcept {
    StringStream ss;
    event.logger_name      = name_;
    event.datetime_pattern = datetime_pattern_;
    for (auto& i : formatters_) {
        i(ss, &event);
    }
    for (int i = 0; i < flusher_count_; ++i) {
        flushers_[i]->flush(ss.str());
    }
}

void Logger::registerUpdateFlusher() const {
}

void Logger::setFormatters(const String& formatter_pattern) {
    if (!formatters_.empty())
        formatters_.clear();

    auto vec = logPatternParse(formatter_pattern);

    //%m 消息体
    //%p level
    //%r 启动后的时间
    //%c 日志名称
    //%t 线程id
    //%n 回车换行
    //%d 时间
    //%f 文件名
    //%l 行号

    for (auto& i : vec) {
        if (std::get<2>(i) == 0) {
            StringLogFormatter string_formatter(std::get<0>(i));
            addOneFormatter(std::move(string_formatter));
        } else {
            auto str = std::get<0>(i);
            assert(!str.empty());
            char key = str[0];
            if (key == 'd') {
                datetime_pattern_ = std::get<1>(i).empty()
                    ? "%Y-%m-%d %H:%M:%S"
                    : std::move(std::get<1>(i));
            }
            auto formatter = LogFormatterFactory::getFormatter(key);
            if (formatter == nullptr) {
                StringLogFormatter string_formatter("<<error_format%" + str +
                    ">>");
                addOneFormatter(std::move(string_formatter));
            } else {
                addOneFormatter(std::move(formatter));
            }
        }
    }
}

_LogManager::_LogManager() {
    initDefaultLogger();
    initLoggerFromConfig();
}

void _LogManager::initDefaultLogger() {
    auto ptr = std::make_shared<Logger>("root");
    ptr->setFormatters(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T<%f:%l>%T%m%n");
    ptr->addOneFlusher(log::FlusherManager::getInstance()->getLogFlusher(
        "ProtectedStdoutFlusher",
        ""));
    ptr->addOneFlusher(log::FlusherManager::getInstance()->getLogFlusher(
        "ProtectedFileFlusher",
        "./logs/%H-root-%P.log"));
    ptr->setLevel(Level::DEBUG);
    default_logger_ = ptr;
    loggers_.insert({"root", ptr});

}

void _LogManager::initLoggerFromConfig() {
    auto log_data =
        BaseMainConfig::getInstance()->get<std::vector<detail::LogConfigData>>(
            "logs");
    for (auto& item : log_data) {
        auto ptr = std::make_shared<Logger>(item.name);
        ptr->setFormatters(item.formatter);
        for (auto flusher : item.flushers) {
            if (auto flusher_ptr = lon::log::FlusherManager::getInstance()->
                getLogFlusher(
                    flusher.type,
                    flusher.name_pattern); flusher_ptr) {
                ptr->addOneFlusher(std::move(flusher_ptr));
            } else {
                std::cerr << "flusher type error, type name:" << flusher.type;
            }

        }
        
        loggers_.insert({item.name, ptr});
        if (item.name == "root") {
            // root logger in config will replace
            // default root logger.
            default_logger_ = ptr;
        }
    }
}


// Logger::ptr LogFactory::default_logger_ = nullptr;
// std::unordered_map<String, Logger::ptr> LogFactory::loggers_{};
namespace detail {

// void initDefaultLogger(_LogManager* manager) {
//     auto ptr = std::make_shared<Logger>("root");
//     ptr->setFormatters(
//         "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T<%f:%l>%T%m%n");
//     ptr->addOneFlusher(log::FlusherManager::getInstance()->getLogFlusher(
//         "ProtectedStdoutFlusher",
//         ""));
//     ptr->addOneFlusher(log::FlusherManager::getInstance()->getLogFlusher(
//         "ProtectedFileFlusher",
//         "./logs/%H-root-%P.log"));
//     ptr->setLevel(Level::DEBUG);
//     manager->default_logger_ = ptr;
//     manager->loggers_.insert({"root", ptr});

//     // LogManager::getInstance()->default_logger_ = ptr;
//     // LogManager::getInstance()->loggers_.insert({"root", ptr});
//     // LogFactory::default_logger_ = ptr;
//     //P
//     // LogFactory::loggers_.insert({ "root", ptr });
// }

// void initLoggerFromConfig(_LogManager* manager) {
//     auto log_data =
//         BaseMainConfig::getInstance()->get<std::vector<detail::LogConfigData>>(
//             "logs");
//     for (auto& item : log_data) {
//         auto ptr = std::make_shared<Logger>(item.name);
//         ptr->setFormatters(item.formatter);
//         for (auto flusher : item.flushers) {
//             if (auto flusher_ptr = lon::log::FlusherManager::getInstance()->
//                 getLogFlusher(
//                     flusher.type,
//                     flusher.name_pattern); flusher_ptr) {
//                 ptr->addOneFlusher(std::move(flusher_ptr));
//             } else {
//                 std::cerr << "flusher type error, type name:" << flusher.type;
//             }

//         }
//         // LogManager::getInstance()->loggers_.insert({item.name, ptr});
//         // if (item.name == "root") {
//         //     // root logger in config will replace
//         //     // default root logger.
//         //     LogManager::getInstance()->default_logger_ = ptr;
//         // }
//         manager->loggers_.insert({item.name, ptr});
//         if (item.name == "root") {
//             // root logger in config will replace
//             // default root logger.
//             manager->default_logger_ = ptr;
//         }
//     }
// }


// struct ___LoggerIniter
// {
//     ___LoggerIniter() {
//         initDefaultLogger();
//         initLoggerFromConfig();
//     }
// };

// ___LoggerIniter initer;
} // namespace detail


} // namespace lon
