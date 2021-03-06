#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include "print_helper.h"
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


template <typename ContainerType,
    typename Type = typename ContainerType::value_type,
    typename = std::enable_if_t<lon::IsCoutable<Type>::value>>
    String join(StringPiece _break,const ContainerType& container) {
    if(container.empty()) return "";

    using StringStream = std::stringstream;
    StringStream ss;
    for(size_t pos = 0; pos < container.size() - 1; ++pos) {
        ss << container[pos] << _break;
    }
    ss<<container.back();
    return ss.str();
}


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
