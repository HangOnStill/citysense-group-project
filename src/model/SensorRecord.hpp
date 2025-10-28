#pragma once
#include <string>
#include <optional>
#include <chrono>

namespace model {

struct SensorRecord {
    std::chrono::system_clock::time_point ts{};
    std::string sensor_id;
    int zone_id{0};

    std::optional<double> speed;
    std::optional<double> flow;
    std::optional<double> pm25;
    std::optional<double> pm10;
    std::optional<double> db;
};

} // namespace model
