// src/core/Finding.hpp
#pragma once
#include <vector>
#include <mutex>
#include <unordered_map>
#include <string>
#include <chrono>

#include "Window.hpp"
#include "Detector.hpp"   // NEW: ensures core::Detector is visible

namespace core {

    struct Finding {
        std::string detector;
        double value{};
        std::unordered_map<std::string, double> thresholds;

        std::chrono::system_clock::time_point start_ts{};
        std::chrono::system_clock::time_point end_ts{};
    };

} // namespace core
