#pragma once
#include <vector>
#include "../model/SensorRecord.hpp"

namespace core {

// Very small window container. Students should implement time-based eviction.
struct Window {
    std::vector<model::SensorRecord> records;
};

} // namespace core
