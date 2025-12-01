#pragma once
#include <chrono>

namespace timeutil {

    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

    inline TimePoint floor_to_minute(TimePoint ts) {
        using namespace std::chrono;
        auto secs = duration_cast<seconds>(ts.time_since_epoch());
        auto mins = duration_cast<minutes>(secs);
        return TimePoint{ mins };
    }

    inline int minutes_since_epoch(TimePoint ts) {
        using namespace std::chrono;
        auto mins = duration_cast<minutes>(ts.time_since_epoch());
        return static_cast<int>(mins.count());
    }

} // namespace timeutil
