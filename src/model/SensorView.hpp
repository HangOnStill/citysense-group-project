#pragma once
#include <string_view>
#include <optional>
#include <chrono>

namespace model {

// Non-owning view over a record parsed from a buffer.
// Students must ensure lifetimes (no dangling views).
struct SensorView {
    std::chrono::system_clock::time_point ts{};
    std::string_view sensor_id;
    int zone_id{0};

    std::optional<double> speed;
    std::optional<double> flow;
    std::optional<double> pm25;
    std::optional<double> pm10;
    std::optional<double> db;
};

} // namespace model
