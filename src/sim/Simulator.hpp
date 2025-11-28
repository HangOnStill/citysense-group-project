#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <sstream>


#include "Clock.hpp"
#include "SeededRNG.hpp"
#include "SimulatorProfile.hpp"
#include "Checkpoint.hpp"
#include "../model/SensorRecord.hpp"

namespace sim{
class Simulator{
    public:
        Simulator(Clock clock, SeededRNG rng);

        void start(SimulatorProfile profile);
        void pause();
        void resume();
        bool isrunning() const {return running_;}

        std::vector<model::SensorRecord> next_step();
        std::vector<model::SensorRecord> next_batch(int steps);

        void save_state(const std::string& path) const;
        void load_state(const std::string& path);

        //Generates one full day of data
        std::vector<model::SensorRecord> generate_day(SimulatorProfile profile);
        //Generates one full day of data
        std::vector<model::SensorRecord> generate_month(SimulatorProfile profile, int month);
        //Generates one full year of data
        std::vector<model::SensorRecord> generate_year(SimulatorProfile profile);
    
    private:
        Clock clock_;
        SeededRNG rng_;

        double last_pm25_glebe_     = 20.0;
        double last_pm25_downtown_  = 20.0;
        double last_pm25_byward_    = 20.0;

        double last_noise_glebe_    = 50.0;
        double last_noise_downtown_ = 50.0;
        double last_noise_byward_   = 50.0;

        bool running_ = false;
        SimulatorProfile profile_{SimulatorProfile::Weekday};

        std::vector<model::SensorRecord> generate_step(SimulatorProfile profile);


        //Helpers to simulate records
        model::SensorRecord generate_air_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, 
            SimulatorProfile profile);

        model::SensorRecord generate_traffic_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, 
            SimulatorProfile profile);

        model::SensorRecord generate_noise_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, 
            SimulatorProfile profile);
};
}