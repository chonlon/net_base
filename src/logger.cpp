#include "logger.h"
#include "logging/logger_flusher.h"
#include "base/info.h"
#include "logging/loger_formatters.h"


#include <iostream>
#include <fmt/core.h>


namespace lon {


Level Logger::levelFromString(StringPiece str) noexcept {
    std::string str_upper;
    std::transform(
        str.begin(),
        str.end(),
        std::back_inserter(str_upper),
        ::toupper);

#define LON_XX(name)            \
    if (str_upper == #name) {   \
        return Level::name;     \
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

static int count =0;
struct __Runner
{
    ~__Runner() { fmt::print("count: {}", count); }
};
__Runner run;

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
    ++count;
}

void Logger::registerUpdateFlusher() const {
}

void Logger::setFormatters(const String& formatter_pattern) {
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr{};
    for (std::size_t i = 0, size = formatter_pattern.size(); i < size; ++i) {
        if (formatter_pattern[i] != '%') {
            nstr.append(1, formatter_pattern[i]);
            continue;
        }
        if ((i + 1) < size) {
            if (formatter_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        std::size_t n         = i + 1;
        int fmt_status        = 0;
        std::size_t fmt_begin = 0;
        std::string str;
        std::string fmt;

        while (n < formatter_pattern.size()) {
            if (!fmt_status &&
                (!isalpha(formatter_pattern[n]) && formatter_pattern[n] != '{'
                    &&
                    formatter_pattern[n] != '}')) {
                str = formatter_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if (fmt_status == 0) {
                if (formatter_pattern[n] == '{') {
                    str        = formatter_pattern.substr(i + 1, n - i - 1);
                    fmt_status = 1; //解析格式
                    fmt_begin  = n;
                    ++n;
                    continue;
                }
            } else if (fmt_status == 1) {
                if (formatter_pattern[n] == '}') {
                    fmt = formatter_pattern.substr(
                        fmt_begin + 1,
                        n - fmt_begin - 1);
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if (n == formatter_pattern.size()) {
                if (str.empty()) {
                    str = formatter_pattern.substr(i + 1);
                }
            }
        }

        if (fmt_status == 0) {
            if (!nstr.empty()) {
                vec.emplace_back(nstr, "", 0);
                nstr.clear();
            }
            // str = formatter_pattern.substr(i + 1, n - i - 1);
            vec.emplace_back(str, fmt, 1);
            i = n - 1;
        } else if (fmt_status == 1) {
            std::cout << "pattern parse error: " << formatter_pattern << " - "
                << formatter_pattern.substr(i) << '\n';
            vec.emplace_back("<<pattern_error>>", fmt, 0);
        }
    }

    if (!nstr.empty()) {
        vec.emplace_back(nstr, "", 0);
    }


    //%m 消息体
    //%p level
    //%r 启动后的时间
    //%c 日志名称
    //%t 线程id
    //%n 回车换行
    //%d 时间
    //%f 文件名
    //%l 行号

    for (auto i : vec) {
        auto [i1,i2,i3] = i;
        fmt::print("{},{},{}\n", i1, i2, i3);
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
                StringLogFormatter string_formatter(
                    "<<error_format%" + str + ">>");
                addOneFormatter(std::move(string_formatter));
            } else {
                addOneFormatter(std::move(formatter));
            }
        }
    }

    //FIXME fix this
    //
    // for (auto& i : vec) {
    //     if (std::get<2>(i) == 0) {
    //         m_items.emplace_back(
    //             std::make_shared<StringFormatItem>(std::get<0>(i)));
    //     }
    //     else {
    //         auto it = s_format_items.find(std::get<0>(i));
    //         if (it == s_format_items.end()) {
    //             m_items.emplace_back(std::make_shared<StringFormatItem>(
    //                 "<<error_format%" + std::get<0>(i) + ">>"));
    //             m_error = true;
    //         }
    //         else {
    //             m_items.emplace_back(it->second(std::get<1>(i)));
    //         }
    //     }
    // }
}
} // namespace lon
