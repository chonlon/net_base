#include "logger_filename.h"


#include "config.h"
#include "base/string_parsers.h"


#include <fmt/compile.h>

namespace lon {


LogFilenameData logFileNameParse(const String& pattern) {
    auto vec = logPatternParse(pattern);
    String prefix;
    String postfix;
    String dateTime;

    prefix.reserve(pattern.size());
    postfix.reserve(10);

    bool in_pre = true;

    for (auto& i : vec) {
        if (std::get<2>(i) == 0) {
            in_pre ? prefix += std::get<0>(i) : postfix += std::get<0>(i);
        } else {
            auto str = std::get<0>(i);
            assert(!str.empty());
            char key = str[0];

            if (key == 'd') {
                dateTime += std::get<1>(i);
                in_pre = false;
                continue;
            }
            if (key == 'H') {
                in_pre ? prefix += getHostWithoutBuffer() : postfix += getHostWithoutBuffer(); //由于初始化顺序不好控制, 只能使用不带buffer的
                continue;
            }
            if (key == 'P') {
                in_pre
                    ? prefix += std::to_string(getPid())
                    : postfix += std::to_string(getPid());
                continue;
            }

            in_pre ? prefix += "_error_pattern_" : postfix += "_error_pattern_";
        }
    }
    return {prefix, postfix, dateTime};
}

String logFileNameGenerate(const LogFilenameData& filename_data) {
    String filename;
    filename.reserve(filename_data.prefix.size() + filename_data.postfix.size() + 15);

    char timebuf[32];
    struct tm tm;
    time_t now = time(NULL);
    localtime_r(&now, &tm);
    strftime(timebuf, sizeof timebuf, filename_data.dateTime.data(), &tm);
    filename += filename_data.prefix;
    filename += timebuf;
    filename += filename_data.postfix;
    return filename;
}

}
