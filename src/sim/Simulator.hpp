#pragma once

#include <vector>
#include <string>
#include <chrono>

#include "Clock.hpp"
#include "SeededRNG.hpp"
#include "SimulatorProfile.hpp"

#include "../model/SensorRecord.hpp"

namespace sim{
class Simulator{
    public:
        Simulator(Clock clock, SeededRNG rng);

        //Generates one full day of data
        std::vector<model::SensorRecord> generate_day(SimulatorProfile profile);
    
    private:
        Clock clock_;
        SeededRNG rng_;

        //Helpers to simulate records
        model::SensorRecord generate_air_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, 
            SimulatorProfile profile);

        model::SensorRecord generate_traffic_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, 
            SimulatorProfile profile);

        model::SensorRecord generate_noise_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, 
            SimulatorProfile profile);
};
}