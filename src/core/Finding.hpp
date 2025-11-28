#pragma once
#include <vector>
#include <mutex>
#include "Window.hpp"
#include <unordered_map>

namespace core {

    struct Finding {
        std::string detector;
        double value;
        std::unordered_map<std::string, double> thresholds;

        std::chrono::system_clock::time_point start_ts;
        std::chrono::system_clock::time_point end_ts;
    };

}