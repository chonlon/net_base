#pragma once

#include <cstdint>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <thread>


namespace lon {
using String = std::string;
using StringPiece = std::string_view;

using Thread = std::thread;
using Mutex = std::mutex;
using RWMutex = std::shared_mutex;

template <typename T>
using ReadLocker = std::shared_lock<T>;
template<typename T>
using WriteLocker = std::unique_lock<T>;

using ConditionVar = std::condition_variable;

using size_t = std::size_t;
using int8_t = std::int8_t;
using uint8_t = std::uint8_t;
using int16_t = std::int16_t;
using uint16_t = std::uint16_t;
using int32_t = std::int32_t;
using uint32_t = std::uint32_t;
using int64_t = std::int64_t;
using uint64_t = std::uint64_t;

namespace data {
	constexpr uint32_t K = 1024;
	constexpr uint32_t M = K * K;
	constexpr uint32_t G = M * K;
}

}