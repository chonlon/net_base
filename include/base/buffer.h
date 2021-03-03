#pragma once
#include <string>

namespace lon {
enum BUFFER_SIZE : uint32_t
{
    kSmallBufferSize = 4096,
    kLargeBufferSize = kSmallBufferSize * 1024
};

template <uint32_t SIZE>
class FixedBuffer
{};

using SmallBuffer = FixedBuffer<kSmallBufferSize>;
using LargeBuffer = FixedBuffer<kSmallBufferSize>;


// 实现一个和folly small vector类似的buffer类, 可以做到自动切换栈/堆内存
template <uint32_t SIZE>
class AutoBuffer
{};

using SmallAutoBuffer = AutoBuffer<kSmallBufferSize>;

}  // namespace lon
