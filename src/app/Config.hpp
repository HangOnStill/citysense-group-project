#pragma once
#include <string>
#include <vector>
#include <chrono>

namespace app {

struct Config {
    std::vector<std::string> inputs;
    std::string out_path{"citysense_report.json"};

    // simulation
    std::chrono::system_clock::time_point start_time{};
    std::chrono::system_clock::time_point end_time{};
    int time_step_seconds{60};
    int report_every_ticks{1};
    int window_minutes{5};

    // detectors (defaults; students may load from JSON)
    double traffic_speed_threshold{20.0};
    int consec_minutes{5};
    double pm25_threshold{35.0};
    double db_threshold{80.0};
    int noise_count_threshold{3};
    int noise_window_minutes{10};
};

} // namespace app
