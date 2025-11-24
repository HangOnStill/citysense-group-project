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
        const std::vector<std::string> traffic_sensors = {"traffic-glebe", "traffic-downtown", "traffic-byward"};
        const std::vector<std::string> air_sensors = {"air-glebe", "air-downtown", "air-byward"};
        const std::vector<std::string> noise_sensors = {"noise-glebe", "noise-downtown", "noise-byward"};

        //Example Zones
        constexpr int ZONE_DOWNTOWN = 1;
        constexpr int ZONE_GLEBE    = 2;
        constexpr int ZONE_BYWARD   = 3;

        //Loops for 24 hours, regardless of time step used in clock
        while (clock_.now() < end_time){
            auto ts = clock_.now();
            int minute = duration_cast<minutes>(ts - start_time).count();

            //Traffic being generated every step size (usually a minute)
            output.push_back(generate_traffic_record(ts, ZONE_GLEBE, traffic_sensors[0], profile));
            output.push_back(generate_traffic_record(ts, ZONE_DOWNTOWN, traffic_sensors[1], profile));
            output.push_back(generate_traffic_record(ts, ZONE_BYWARD, traffic_sensors[2], profile));
            

            //Air quality being generated every step size
            output.push_back(generate_air_record(ts, ZONE_GLEBE, air_sensors[0], profile));
            output.push_back(generate_air_record(ts, ZONE_DOWNTOWN, air_sensors[1], profile));
            output.push_back(generate_air_record(ts, ZONE_BYWARD, air_sensors[2], profile));

            //Noise being generated every step size
            output.push_back(generate_noise_record(ts, ZONE_GLEBE, noise_sensors[0], profile));
            output.push_back(generate_noise_record(ts, ZONE_DOWNTOWN, noise_sensors[1], profile));
            output.push_back(generate_noise_record(ts, ZONE_BYWARD, noise_sensors[2], profile));
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


        double base_min;
        double base_max;

        switch(zone_id){
            case 1: // Downtown
            base_min = 35.0; 
            base_max = 55.0;
            break;
        case 2: // Byward Market
            base_min = 30.0;
            base_max = 50.0;
            break;
        case 3: // Glebe
        default:
            base_min = 25.0;
            base_max = 45.0;
            break;
        }
        double speed = rng_.uniform(base_min, base_max);

        if (is_weekday) {
            double rush_slowdown_min;
            double rush_slowdown_max;
            if (hour >= 7 && hour <= 9) {  // AM rush         
                switch(zone_id){
                    // Downtown rush hour slows traffic more than Byward and Glebe, Byward slows less than Downtown but more than Glebe, etc
                    case 1: rush_slowdown_min = -25; rush_slowdown_max = -12; break;
                    case 2: rush_slowdown_min = -20; rush_slowdown_max = -10; break; 
                    case 3: rush_slowdown_min = -15; rush_slowdown_max = -8;  break; 
                }
            } else if (hour >= 16 && hour <= 18) {       // PM rush
                switch(zone_id){
                    // Downtown rush hour slows traffic more than Byward and Glebe, Byward slows less than Downtown but more than Glebe, etc
                    case 1: rush_slowdown_min = -20; rush_slowdown_max = -10; break;
                    case 2: rush_slowdown_min = -15; rush_slowdown_max = -8; break; 
                    case 3: rush_slowdown_min = -10; rush_slowdown_max = -5;  break; 
                }
            }
            speed += rng_.uniform(rush_slowdown_min, rush_slowdown_max);
        } else { // Weekend
           //Slowdown near Byward lunch & clubs
            if (zone_id == 2 && hour >= 11 && hour <= 13) {
                speed += rng_.uniform(-8.0, 0.0);
            }
            if (zone_id == 2 && hour >= 20 && hour <= 23) {
                speed += rng_.uniform(-12.0, -3.0);
            }
        }
        speed = std::clamp(speed, 5.0, 70.0);
        output.speed = speed;
        
        double flow;
        if (speed < 20) flow = rng_.uniform(20, 35);
        else if (speed < 35) flow = rng_.uniform(15, 25);
        else flow = rng_.uniform(8, 18);

        output.flow = flow;
        return output;
    }

    SensorRecord Simulator::generate_air_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, SimulatorProfile profile){
        SensorRecord output{};
        output.ts = ts;
        output.zone_id = zone_id;
        output.sensor_id = sensor_id;

        int hour = hour_of_day(ts);

        double min_step = -1.5;
        double max_step = 1.5;

        if (hour >= 7 && hour <= 9) {
        min_step = -1.0;
         max_step = 5.0; // morning pollution bump
        }

        double delta = rng_.uniform(min_step, max_step);
        last_pm25_ += delta;

        last_pm25_ = std::clamp(last_pm25_, 5.0, 100.0);
        double pm10 = last_pm25_ * rng_.uniform(1.3, 1.8);

        output.pm25 = last_pm25_;
        output.pm10 = pm10;
        return output;
    }

    SensorRecord Simulator::generate_noise_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, SimulatorProfile profile){
        SensorRecord output{};
        output.ts = ts;
        output.sensor_id = sensor_id;
        output.zone_id = zone_id;

        int hour = hour_of_day(ts);
        double min_step = -2.0;
        double max_step = 2.5;

        if (hour >= 7 && hour <= 9) {
            min_step = -1.0;
            max_step =  5.0;      // weekday morning spike
        }

        if (profile == SimulatorProfile::Weekend && hour >= 20 && hour <= 23) {
            min_step =  0.0;
            max_step =  7.0;     // nightlife effect
        }

        double delta = rng_.uniform(min_step, max_step);
        last_noise_db_ += delta;
        last_noise_db_ = std::clamp(last_noise_db_, 35.0, 95.0);
        output.db = last_noise_db_;
        return output;
    }
}