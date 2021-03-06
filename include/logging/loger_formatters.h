#pragma once
#include "base/singleton.h"
#include "logger.h"
#include "base/typedef.h"

// #include "base/lstring.h"


namespace lon {

class LogFormatterFactory
{
public:
    LogFormatterFactory()  = delete;
    ~LogFormatterFactory() = delete;
    static Logger::FormatterFunc getFormatter(char keyword) noexcept;
};

//string formatter 需要额外构造string, 所以使用函数对象提供
class StringLogFormatter
{
public:
    explicit StringLogFormatter(const String& _str)
        : str_{_str} {
    }

    void operator()(Logger::StringStream& stream, LogEvent*) const {
        stream << str_;
    }

private:
    String str_;
};

//using LogFormatterFactory = Singleton<_LogFormatterFactory>;

}
