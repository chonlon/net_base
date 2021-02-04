#pragma once
#include "base/nocopyable.h"
#include "base/typedef.h"

namespace lon {

struct LogFilename
{
    String prefix;
    String postfix;
    String dateTime;
};

/**
 * @brief 分析filename的pattern.
 * @param pattern
 * %d 时间格式
 * %H hostname
 * %P pid
 * @return tuple<0> prefix-before datetime
 * @return tuple<1> postfix-after datetime
 * @return tuple<2> datetime_pattern
*/
LogFilename logFileNameParse(const String& pattern);

String logFileNameGenerate(const String& prefix,
                           const String& postfix,
                           const String& datetime_pattern);

}
