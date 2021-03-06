#pragma once
#include <type_traits>

/**
 * @brief 使用梅森缠绕器生成从[begin,end]随机数
 * @tparam T int type
 * @param begin 
 * @param end 
 * @return 产生的随机数.
*/
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>, T>>
T mt19937RandomGen(T begin, T end) {
    std::random_device rd;  // 将用于为随机数引擎获得种子
    std::mt19937 gen(rd()); // 以播种标准 mersenne_twister_engine
    std::uniform_int_distribution<T> u(begin, end);
    return u(gen);
}
