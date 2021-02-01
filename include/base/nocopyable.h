#pragma once

namespace lon {
class Noncopyable
{
public:
    Noncopyable() = default;
    ~Noncopyable() = default;
private:
    Noncopyable(const Noncopyable& other) = delete;
    Noncopyable(Noncopyable&& other) noexcept = delete;
    auto operator=(const Noncopyable& other) -> Noncopyable& = delete;
    auto operator=(Noncopyable&& other) noexcept -> Noncopyable& = delete;
};

}
