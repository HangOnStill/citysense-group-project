// src/sim/Simulator.hpp
#pragma once

#include <vector>
#include <string>
#include <chrono>

#include "sim/Clock.hpp"
#include "sim/SeededRNG.hpp"
#include "sim/SimulatorProfile.hpp"
#include "sim/Checkpoint.hpp"
#include "model/SensorRecord.hpp"

namespace sim {

    class Simulator {
    public:
        Simulator(Clock& clock, SeededRNG rng);

        void start(SimulatorProfile profile);
        void pause();
        void resume();
        bool is_running() const { return running_; }

        std::vector<model::SensorRecord> next_step();
        std::vector<model::SensorRecord> next_batch(int steps);

        void save_state(const std::string& path) const;
        void load_state(const std::string& path);

        // Generate synthetic data
        std::vector<model::SensorRecord> generate_day(SimulatorProfile profile);
        std::vector<model::SensorRecord> generate_month(SimulatorProfile profile, int month);
        std::vector<model::SensorRecord> generate_year(SimulatorProfile profile);

    private:
        Clock& clock_;
        SeededRNG rng_;

        double last_pm25_glebe_ = 20.0;
        double last_pm25_downtown_ = 20.0;
        double last_pm25_byward_ = 20.0;

        double last_noise_glebe_ = 50.0;
        double last_noise_downtown_ = 50.0;
        double last_noise_byward_ = 50.0;

        bool             running_ = false;
        SimulatorProfile profile_{ SimulatorProfile::Weekday };

        std::vector<model::SensorRecord> generate_step(SimulatorProfile profile);

        model::SensorRecord generate_air_record(
            std::chrono::system_clock::time_point ts,
            int zone_id,
            const std::string& sensor_id,
            SimulatorProfile profile
        );

        model::SensorRecord generate_traffic_record(
            std::chrono::system_clock::time_point ts,
            int zone_id,
            const std::string& sensor_id,
            SimulatorProfile profile
        );

        model::SensorRecord generate_noise_record(
            std::chrono::system_clock::time_point ts,
            int zone_id,
            const std::string& sensor_id,
            SimulatorProfile profile
        );
    };

    // Free helper to dump simulated records to CSVs
    void write_csv_files(
        const std::vector<model::SensorRecord>& records,
        const std::string& traffic_file,
        const std::string& air_file,
        const std::string& noise_file
    );

} // namespace sim
