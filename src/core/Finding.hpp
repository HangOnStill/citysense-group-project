// src/core/Finding.hpp
#pragma once

#include <string>
#include <unordered_map>
#include <chrono>

namespace core {

    struct Finding {
        // Which detector produced this finding
        std::string detector;

        // Short type/category, e.g. "high-noise", "high-pm25"
        std::string type;

        // Human-readable explanation
        std::string message;

        // Where it happened
        int         zone_id{};        // 0 if not applicable
        std::string sensor_id;        // empty if not applicable

        // Representative timestamp of the event
        std::chrono::system_clock::time_point timestamp{};

        // Window coverage
        std::chrono::system_clock::time_point start_ts{};
        std::chrono::system_clock::time_point end_ts{};

        // Main numeric score/value (e.g., max dB, mean speed)
        double value{};               // default 0

        // Thresholds used by the detector
        std::unordered_map<std::string, double> thresholds;
    };

} // namespace core
