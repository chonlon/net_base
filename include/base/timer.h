#pragma once
#include "info.h"
#include "nocopyable.h"
#include "typedef.h"


#include <cassert>
#include <functional>
#include <set>


namespace lon {
struct Timer
{
    using CallbackType = std::function<void()>;
    using MsStampType  = size_t;
    using Ptr          = std::shared_ptr<Timer>;

    struct Comparator
    {
        bool operator()(Timer::Ptr lhs, Timer::Ptr rhs) const {
            assert(lhs && rhs);
            return lhs->target_timestamp < rhs->target_timestamp;
        }
    };


    Timer(const Timer& _other)     = default;
    Timer(Timer&& _other) noexcept = default;
    auto operator=(const Timer& _other) -> Timer& = default;
    auto operator=(Timer&& _other) noexcept -> Timer& = default;
    ~Timer()                                          = default;

    Timer(MsStampType _interval, CallbackType _callback, bool _repeat = false)
        : repeat{_repeat}, interval{_interval}, callback{std::move(_callback)} {
        constexpr auto i = static_cast<MsStampType>(-1);
        auto cur_ms = currentMs();
        if (interval > static_cast<MsStampType>(-1) - cur_ms)
            target_timestamp = static_cast<MsStampType>(-1);
        else
            target_timestamp = interval + cur_ms;
    }

    Timer(MsStampType _target_timestamp)
        : target_timestamp{_target_timestamp} {}

    bool repeat                  = false;
    MsStampType target_timestamp = static_cast<MsStampType>(-1);
    MsStampType interval         = 0;
    CallbackType callback        = nullptr;
};

/**
 * @brief Timer是否过期管理器, 用在有计时功能的地方管理计时器.
 */
class TimerManager : public Noncopyable
{
public:
    /**
     * @brief add timer to manager, timer callback not null required.
     * @param timer timer shared ptr, not null required.
     */
    void addTimer(Timer::Ptr timer) {
        assert(timer);
        assert(timer->callback != nullptr);
        std::lock_guard<Mutex> locker(timer_mutex_);
        timers_.insert(std::move(timer));
    }

    /**
     * @brief 获取所有过期定时器.
     * @return 过期定时器列表.
     */
    std::vector<Timer::Ptr> takeExpiredTimers() {
        Timer::MsStampType cur_ms = currentMs();
        Timer::Ptr current_timer  = std::make_shared<Timer>(cur_ms);
        std::vector<Timer::Ptr> result;

        {
            std::lock_guard<Mutex> locker(timer_mutex_);
            auto iter = timers_.lower_bound(current_timer);
            while (iter != timers_.end() &&
                   (*iter)->target_timestamp == cur_ms) {
                ++iter;
            }
            std::move(timers_.begin(), iter, std::back_inserter(result));
            timers_.erase(timers_.begin(), iter);
            for (auto& timer : result) {
                if (timer->repeat) {
                    timers_.insert(std::make_shared<Timer>(
                        timer->interval, timer->callback, true));
                }
            }
        }

        return result;
    };

    Timer::MsStampType getNextInterval() const {
        std::lock_guard<Mutex> locker(timer_mutex_);
        if (timers_.empty())
            return static_cast<Timer::MsStampType>(-1);
        if((*timers_.begin())->target_timestamp < currentMs()) {
            return 0;
        }
        return (*timers_.begin())->target_timestamp - currentMs();
    }

    /**
     * @brief 获取指定时间的首个过期定时器.
     * @param [in] timer  如果存在过期定时器, 返回timer指针,
     * 否则timer指针不被设置.
     * @param cur_ms 当前时间.
     * @return true如果存在, 否则返回false.
     */
    bool takeFirstIfExpired(Timer::Ptr& timer, Timer::MsStampType cur_ms) {

        std::lock_guard<Mutex> locker(timer_mutex_);
        if (timers_.empty())
            return false;
        if ((*timers_.begin())->target_timestamp <= cur_ms) {
            timer = std::move(*timers_.begin());
            timers_.erase(timers_.begin());
            if (timer->repeat) {
                timers_.insert(std::make_shared<Timer>(
                    timer->interval, timer->callback, true));
            }
            if (!timer->callback) {
                return false;
            }
            return true;
        } else {
            return false;
        }
    }

    void removeTimer(Timer::Ptr timer) {
        if (!timer)
            return;
        std::lock_guard<Mutex> locker(timer_mutex_);
        auto iter = timers_.find(timer);
        timers_.erase(iter);
    }

private:
    mutable Mutex timer_mutex_{};
    std::multiset<Timer::Ptr, Timer::Comparator> timers_{};
};
}  // namespace lon
