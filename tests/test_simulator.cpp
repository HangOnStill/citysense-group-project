//This is just a minimal tester I made to see if determinism was correct with my simulator
#include <iostream>
#include <chrono>
#include "sim/Simulator.hpp"
#include "sim/SimulatorProfile.hpp"

int main() {
    using namespace std::chrono;

    system_clock::time_point start{seconds{0}};
    sim::Clock clock{start, 60};
    sim::SeededRNG rng{1234};
    sim::Simulator sim{clock, rng};

    auto day = sim.generate_day(sim::SimulatorProfile::Weekday);

    // Print simple CSV-ish lines; you can trim fields if you want
    for (const auto& rec : day) {
        auto mins_since_start =
            duration_cast<minutes>(rec.ts - day.front().ts).count();

        std::cout << rec.ts << ','
          << rec.zone_id << ','
          << rec.sensor_id << ','
          << rec.speed.value_or(-1.0) << ','
          << rec.flow.value_or(-1.0) << ','
          << rec.pm25.value_or(-1.0) << ','
          << rec.pm10.value_or(-1.0) << ','
          << rec.db.value_or(-1.0) << '\n';
    }
}
