#pragma once
#include "../base/nocopyable.h"
#include "../base/typedef.h"

namespace lon {

struct LogFilenameData
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
 * @return prefix --before datetime
 * @return postfix --after datetime
 * @return datetime_pattern
*/
LogFilenameData logFileNameParse(const String& pattern);

String logFileNameGenerate(const LogFilenameData& filename_data);

}
