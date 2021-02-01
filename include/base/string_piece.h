#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "typedef.h"

namespace lon {


inline StringPiece toStringPiece(std::string::iterator begin,
                                 std::string::iterator end) {
    return {&(*begin), static_cast<std::string::size_type>(end - begin)};
};


/**
 * \brief split string piece by split_word
 * \return words split by split_word in str_v
 */
std::vector<StringPiece> splitStringPiece(StringPiece str_v,
                                          StringPiece split_world);

/**
 * \brief 简单字符串参数.
 *
 */
struct StringArg
{
    StringArg(const char* _str) : str(_str) {}
    StringArg(const std::string& _str) : str(_str.c_str()) {}

    const char* str;
};
}  // namespace lon
