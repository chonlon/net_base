﻿#pragma once
#include "info.h"
#include "nocopyable.h"
#include "typedef.h"


#include <functional>
#include <set>
#include <iostream>

namespace lon {
struct Timer
{
    using CallbackType = std::function<void()>;
    using MsStampType  = size_t;

    struct Comparator
    {
        bool operator()(const Timer& lhs, const Timer& rhs) const {
            return lhs.target_timestamp < rhs.target_timestamp;
        }
    };


    Timer(const Timer& _other)     = default;
    Timer(Timer&& _other) noexcept = default;
    auto operator=(const Timer& _other) -> Timer& = default;
    auto operator=(Timer&& _other) noexcept -> Timer& = default;
    ~Timer()                                          = default;

    Timer(MsStampType _interval, CallbackType _callback, bool _repeat = false)
        : repeat{_repeat},
          target_timestamp{_interval + currentMs()},
          interval{_interval},
          callback{std::move(_callback)} {}

    Timer(MsStampType _target_timestamp)
        : target_timestamp{_target_timestamp} {}

    bool repeat                  = false;
    MsStampType target_timestamp = 0;
    MsStampType interval         = 0;
    CallbackType callback        = nullptr;
};

/**
 * @brief Timer是否过期管理器, 用在有计时功能的地方管理计时器.
 */
class TimerManager : public Noncopyable
{
public:
    void addTimer(Timer timer) {
        std::lock_guard<Mutex> locker(timer_mutex_);
        timers_.insert(std::move(timer));
    }

    /**
     * @brief 获取所有过期定时器.
     * @return 过期定时器列表.
     */
    std::vector<Timer> getExpiredTimers() {
        Timer::MsStampType cur_ms = currentMs();
        Timer current_timer(cur_ms);
        std::vector<Timer> result;

        {
            std::lock_guard<Mutex> locker(timer_mutex_);
            auto iter = timers_.lower_bound(current_timer);
            while (iter != timers_.end() && iter->target_timestamp == cur_ms) {
                ++iter;
            }
            std::move(timers_.begin(), iter, std::back_inserter(result));

            for (auto& timer : result) {
                if (timer.repeat) {
                    Timer repeat_timer(timer.interval,
                                       timer.callback,
                                       true);
                    timers_.insert(std::move(repeat_timer));
                }
            }
        }

        return result;
    };

    Timer::MsStampType getNextInterval() const {
        std::lock_guard<Mutex> locker(timer_mutex_);
        if (timers_.empty())
            return 0;
        return timers_.begin()->target_timestamp - currentMs();
    }

    /**
     * @brief 获取指定时间的首个过期定时器.
     * @param [in] timer  如果存在过期定时器, 返回timer指针,
     * 否则timer指针不被设置. 此函数不拥有指针所有权.
     * @param cur_ms 当前时间.
     * @return true如果存在, 否则返回false.
     */
    bool getFirstIfExpired(Timer* timer, Timer::MsStampType cur_ms) {

        std::lock_guard<Mutex> locker(timer_mutex_);
        if (timers_.empty())
            return false;
        if (timers_.begin()->target_timestamp <= cur_ms) {
            *timer = std::move(*timers_.begin());
            timers_.erase(timers_.begin());
            if(timer->repeat) {
                Timer repeat_timer(timer->interval,
                    timer->callback,
                    true);
                timers_.insert(std::move(repeat_timer));
            }
            if(!timer->callback) {
                return false;
            }
            return true;
        } else {
            return false;
        }
    }

private:
    mutable Mutex timer_mutex_;
    std::multiset<Timer, Timer::Comparator> timers_;
};
}  // namespace lon
