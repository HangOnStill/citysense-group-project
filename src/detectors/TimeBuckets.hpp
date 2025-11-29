#pragma once
#include <chrono>

namespace timeutil {

    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

    // Project-wide definition: a "time-step" is 60 seconds; a "bucket"
    // is a closed-open [minute, minute+1min) interval.
    inline TimePoint floor_to_minute(TimePoint ts) {
        using namespace std::chrono;
        auto secs = duration_cast<seconds>(ts.time_since_epoch());
        auto mins = duration_cast<minutes>(secs);
        return TimePoint{ mins };
    }

    // If you need a printable key like "08:00"
    inline int minutes_since_epoch(TimePoint ts) {
        using namespace std::chrono;
        auto mins = duration_cast<minutes>(ts.time_since_epoch());
        return static_cast<int>(mins.count());
    }

} // namespace timeutil
