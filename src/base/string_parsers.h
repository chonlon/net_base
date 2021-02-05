#pragma once
#include <vector>
#include "base/typedef.h"

#include <iostream>

namespace lon {

	/**
	 * @brief 分析日志器使用的字符串pattern
	 * @param formatter_pattern 待分析字串
	 * @return tuple<0> pattern的key, 当无对应模式时, 对应原本普通字串
	 * @return tuple<1> 模式对应的字串, 比方说datetime后面会跟需要指定的时间格式字串
	 * @return tuple<2> 是否有对应模式, 0代表没有, 1代表有
	*/
	inline std::vector<std::tuple<std::string, std::string, int>> logPatternParse(const String& formatter_pattern) {
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

            std::size_t n = i + 1;
            int fmt_status = 0;
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
                        str = formatter_pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1; //解析格式
                        fmt_begin = n;
                        ++n;
                        continue;
                    }
                }
                else if (fmt_status == 1) {
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
            }
            else if (fmt_status == 1) {
                std::cout << "pattern parse error: " << formatter_pattern << " - "
                    << formatter_pattern.substr(i) << '\n';
                vec.emplace_back("<<pattern_error>>", fmt, 0);
            }
        }

        if (!nstr.empty()) {
            vec.emplace_back(nstr, "", 0);
        }

        return vec;
	}
}
