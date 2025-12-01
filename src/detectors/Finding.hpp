#pragma once

#include <optional>
#include <string>

namespace detectors {

    // Generic finding produced by any detector.
    // This is JSON-friendly and easy to log or export.
    struct Finding {
        std::string detector;     // e.g., "TrafficCongestion", "AirAlert", "NoiseSpike"
        int zone_id{ 0 };           // zone where the event occurred
        std::string description;  // human-readable summary

        // Optional numeric metric, e.g. average speed, mean PM2.5, dB level, etc.
        std::optional<double> metric_value{};
    };

} // namespace detectors
