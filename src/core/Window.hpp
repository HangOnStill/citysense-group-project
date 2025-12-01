#pragma once

#include <vector>
#include <chrono>
#include "../model/SensorRecord.hpp"

namespace core {

    // Very small window container. Students should implement time-based eviction.
    struct Window {
        std::vector<model::SensorRecord> records;

        // Timestamp of first record in the current window (optional, but useful).
        std::chrono::system_clock::time_point time_start{};
    };

} // namespace core
