#pragma once
#include <chrono>

namespace sim {

class Clock {
    std::chrono::system_clock::time_point now_;
    int step_s_;
public:
    Clock(std::chrono::system_clock::time_point start, int step_seconds)
      : now_(start), step_s_(step_seconds) {}
    std::chrono::system_clock::time_point now() const { return now_; }
    void advance() { now_ += std::chrono::seconds(step_s_); }
};

} // namespace sim
