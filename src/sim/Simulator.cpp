#include <chrono>

#include "Simulator.hpp"

namespace sim{
    using model::SensorRecord;

    //Helper Function - Used to simulate rush hours etc
    static int hour_of_day(std::chrono::system_clock::time_point ts){
        using namespace std::chrono;
        auto secs = duration_cast<seconds>(ts.time_since_epoch()).count();
        long hrs = secs / 3600;
        int h = static_cast<int>(hrs % 24);
        return (h < 0) ? h + 24 : h;
    }

    Simulator::Simulator(Clock clock, SeededRNG rng) : clock_(clock), rng_(rng){}

    //Will Simulate a full day of data, randomised based on a given seed, will be 1 day, regardless of clock step_size
    std::vector<SensorRecord> Simulator::generate_day(SimulatorProfile profile){
        std::vector<SensorRecord> output;
        output.reserve(4000);

        using namespace std::chrono;

        const auto start_time = clock_.now();
        const auto end_time = start_time + hours(24);

        //Example Sensors - Will prob be changed
        const std::vector<std::string> traffic_sensors = {"TRF-SNS-01", "TRF-SNS-02"};
        const std::vector<std::string> air_sensors = {"AIR-SNS-01"};
        const std::vector<std::string> noise_sensors = {"NSE-SNS-01"};

        //Example Zones
        constexpr int ZONE_DOWNTOWN = 1;
        constexpr int ZONE_GLEBE    = 2;
        constexpr int ZONE_BYWARD   = 3;

        //Loops for 24 hours, regardless of time step used in clock
        while (clock_.now() < end_time){
            auto ts = clock_.now();
            int minute = duration_cast<minutes>(ts - start_time).count();

            //Traffic being generated every step size (usually a minute)
            output.push_back(generate_traffic_record(ts, ZONE_DOWNTOWN, traffic_sensors[0], profile));
            output.push_back(generate_traffic_record(ts, ZONE_GLEBE, traffic_sensors[1], profile));

            //Air quality being generated every 5 minutes of simulated time
            if (minute % 5 == 0){
                output.push_back(generate_air_record(ts, ZONE_BYWARD, air_sensors[0], profile));
            }

            //Noise being generated every step size
            output.push_back(generate_noise_record(ts, ZONE_BYWARD, noise_sensors[0], profile));

            clock_.advance();
        }
        return output;

    }

    SensorRecord Simulator::generate_traffic_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, SimulatorProfile profile){
        SensorRecord output{};
        output.ts = ts;
        output.zone_id = zone_id;
        output.sensor_id = sensor_id;
        
        int hour = hour_of_day(ts);
        bool is_weekday = (profile == SimulatorProfile::Weekday); //Return true if generateDay is called with SimulatorProfile that is weekday

        double speed = 0.0;
        double flow = 0.0;
        if (is_weekday) {
            if (hour >= 7 && hour <= 9) {                // AM rush
                speed = rng_.uniform(12.0, 25.0);
                flow      = rng_.uniform(20.0, 35.0);
            } else if (hour >= 16 && hour <= 18) {       // PM rush
                speed = rng_.uniform(15.0, 30.0);
                flow      = rng_.uniform(18.0, 32.0);
            } else {                                     // Normal flow
                speed = rng_.uniform(35.0, 55.0);
                flow      = rng_.uniform(10.0, 20.0);
            }
        } else { // Weekend
            if (hour >= 11 && hour <= 14) {
                speed = rng_.uniform(30.0, 45.0);
                flow      = rng_.uniform(12.0, 22.0);
            } else if (hour >= 20 && hour <= 23) {
                speed = rng_.uniform(28.0, 42.0);
                flow      = rng_.uniform(10.0, 20.0);
            } else {
                speed = rng_.uniform(40.0, 60.0);
                flow      = rng_.uniform(5.0, 15.0);
            }
        }
        output.speed = speed;
        output.flow  = flow;
        return output;
    }

    SensorRecord Simulator::generate_air_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, SimulatorProfile profile){
        SensorRecord output{};
        output.ts = ts;
        output.zone_id = zone_id;
        output.sensor_id = sensor_id;

        int hour = hour_of_day(ts);

        double base_air_quality = (profile == SimulatorProfile::Weekday) ? 25.0 : 20.0;

        if (hour >= 7 && hour <= 9) {
        base_air_quality += 8.0; // morning pollution bump
        }

        double pm25 = base_air_quality + rng_.uniform(-4.0, 4.0);
        double pm10 = pm25 + rng_.uniform(8.0, 18.0);

        output.pm25 = pm25;
        output.pm10 = pm10;
        return output;
    }

    SensorRecord Simulator::generate_noise_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, SimulatorProfile profile){
        SensorRecord output{};
        output.ts = ts;
        output.sensor_id = sensor_id;
        output.zone_id = zone_id;

        int hour = hour_of_day(ts);
        double db = rng_.uniform(40.0, 65.0);  // baseline city noise

        if (hour >= 7 && hour <= 9) {
            db += rng_.uniform(0.0, 5.0);      // weekday morning spike
        }

        if (profile == SimulatorProfile::Weekend && hour >= 20 && hour <= 23) {
            db += rng_.uniform(5.0, 15.0);     // nightlife effect
        }

        output.db = db;
        return output;
    }
}