// dev/demo_simulator.cpp
#include <chrono>
#include <iostream>

#include "sim/Simulator.hpp"
#include "sim/SimulatorProfile.hpp"
#include "sim/PatternAnalytics.hpp"
#include "sim/Criteria.hpp"
#include "sim/Clock.hpp"
#include "sim/SeededRNG.hpp"

using namespace std::chrono;

int main() {
    system_clock::time_point start{ seconds{0} };
    sim::Clock     clock{ start, 60 };
    sim::SeededRNG rng{ 1234 };
    sim::Simulator sim{ clock, rng };
    auto profile = sim::SimulatorProfile::Weekday;
    sim::Criteria criteria;

    auto year_data = sim.generate_year(profile);
    auto year_data_aggregated = sim::analyse_year(year_data, criteria);
    sim::print_year_summary(year_data_aggregated);

    auto month_data = sim.generate_month(profile, 1);
    sim::write_csv_files(year_data,
        "./data/traffic_data.csv",
        "./data/air_data.csv",
        "./data/noise_data.csv");
    auto month_data_aggregated = sim::analyse_month(month_data, criteria);
    sim::print_month_summary(month_data_aggregated);
}
