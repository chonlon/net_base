#include "logging/loger_formatters.h"

namespace lon {

void messageFormatter(Logger::StringStream& stream, LogEvent* event) noexcept {
    stream << event->content;
}

void levelFormatter(Logger::StringStream& stream, LogEvent* event) noexcept {
    stream << Logger::levelToString(event->level);
}

void elapsedFormatter(Logger::StringStream& stream, LogEvent* event) noexcept {
    stream << event->elapsed_ms;
}

void nameFormatter(Logger::StringStream& stream, LogEvent* event) noexcept {
    stream << event->logger_name;
}

void threadIdFormatter(Logger::StringStream& stream, LogEvent* event) noexcept {
    stream << event->thread_id;
}

void newlineFormatter(Logger::StringStream& stream, LogEvent* event) noexcept {
    stream << "\r\n";
}

void dateTimeFormatter(Logger::StringStream& stream, LogEvent* event) noexcept {
    struct tm tm;
    time_t time = event->time;
    localtime_r(&time, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), event->datetime_pattern.data(), &tm);
    stream << buf;
}

void filenameFormatter(Logger::StringStream& stream, LogEvent* event) noexcept {
    stream << event->file_name;
}

void lineNumberFormatter(Logger::StringStream& stream,
                         LogEvent* event) noexcept {
    stream << event->line;
}

void executorIdFormatter(Logger::StringStream& stream, LogEvent* event) noexcept {
    stream << event->executor_id;
}

void tabFormatter(Logger::StringStream& stream, LogEvent* event) noexcept {
    stream << '\t';
}


/**
 * \brief 通过关键字获取formatter
 * \param keyword \n
 * m 消息体 \n
 * p level \n
 * r 启动后的时间\n
 * c 日志名称\n
 * t 线程id\n
 * n 回车换行\n
 * d 时间\n
 * f 文件名\n
 * l 行号\n
 * \return FormatterFunc类型formatter函数对象
 */
Logger::FormatterFunc LogFormatterFactory::getFormatter(
    char keyword) noexcept {
    switch (keyword) {
        case 'm':
            return &messageFormatter;
        case 'p':
            return &levelFormatter;
        case 'r':
            return &elapsedFormatter;
        case 'c':
            return &nameFormatter;
        case 't':
            return &threadIdFormatter;
        case 'n':
            return &newlineFormatter;
        case 'd':
            return &dateTimeFormatter;
        case 'f':
            return &filenameFormatter;
        case 'l':
            return &lineNumberFormatter;
        case 'E':
            return &executorIdFormatter;
        case 'T':
            return &tabFormatter;
        default:
            //TODO log to debug has a unexpected param
            return nullptr;
    }
}

}
