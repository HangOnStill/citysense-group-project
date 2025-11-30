#include <chrono>
#include <fstream>
#include <vector>
#include <string>

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
    Simulator::Simulator(Clock& clock, SeededRNG rng) : clock_(clock), rng_(rng){}

    void Simulator::start(SimulatorProfile profile){
        profile_ = profile;
        running_ = true;

        last_pm25_glebe_     = 20.0;
        last_pm25_downtown_  = 20.0;
        last_pm25_byward_    = 20.0;

        last_noise_glebe_    = 50.0;
        last_noise_downtown_ = 50.0;
        last_noise_byward_   = 50.0;
    }

    void Simulator::pause(){
        running_ = false;
    }
    
    void Simulator::resume(){
        running_ = true;
    }

    //Simulates data from one time step
    std::vector<SensorRecord> Simulator::generate_step(SimulatorProfile profile){
        using namespace std::chrono;
        std::vector<SensorRecord> output;
        output.reserve(9);
        
        auto ts = clock_.now();

        //Example Sensors - Will prob be changed
        const std::vector<std::string> traffic_sensors = {"traffic-glebe", "traffic-downtown", "traffic-byward"};
        const std::vector<std::string> air_sensors = {"air-glebe", "air-downtown", "air-byward"};
        const std::vector<std::string> noise_sensors = {"noise-glebe", "noise-downtown", "noise-byward"};

        //Example Zones
        constexpr int ZONE_DOWNTOWN = 1;
        constexpr int ZONE_GLEBE    = 2;
        constexpr int ZONE_BYWARD   = 3;



        //Generate traffic data, once for each sensor
        output.push_back(generate_traffic_record(ts, ZONE_GLEBE, traffic_sensors[0], profile));
        output.push_back(generate_traffic_record(ts, ZONE_DOWNTOWN, traffic_sensors[1], profile));
        output.push_back(generate_traffic_record(ts, ZONE_BYWARD, traffic_sensors[2], profile));
            

        //Generate air quality data, once for each sensor
        output.push_back(generate_air_record(ts, ZONE_GLEBE, air_sensors[0], profile));
        output.push_back(generate_air_record(ts, ZONE_DOWNTOWN, air_sensors[1], profile));
        output.push_back(generate_air_record(ts, ZONE_BYWARD, air_sensors[2], profile));

        ///Generate noise data, once for each sensor
        output.push_back(generate_noise_record(ts, ZONE_GLEBE, noise_sensors[0], profile));
        output.push_back(generate_noise_record(ts, ZONE_DOWNTOWN, noise_sensors[1], profile));
        output.push_back(generate_noise_record(ts, ZONE_BYWARD, noise_sensors[2], profile));
        clock_.advance();
        return output;
    }

    //Simulates one step, after current step
    std::vector<SensorRecord> Simulator::next_step(){
        if (!running_){
            return{};
        }
        auto output = generate_step(profile_);
        return output;
    }
    
    //Generates x steps of data
    std::vector<SensorRecord> Simulator::next_batch(int steps){
        std::vector<SensorRecord> output;
        output.reserve(steps * 9);

        for (int i = 0; i < steps; ++i) {
            auto step = next_step();
            if (step.empty()) break;
            output.insert(output.end(), step.begin(), step.end());
        }
        return output;
    }

    //Will Generate a full day of data
    std::vector<SensorRecord> Simulator::generate_day(SimulatorProfile profile){
        using namespace std::chrono;
        std::vector<SensorRecord> output;
        output.reserve(9 * 1440);

        const auto start_time = clock_.now();
        const auto end_time   = start_time + hours(24);

        while (clock_.now() < end_time) {
            auto step_records = generate_step(profile);
            output.insert(output.end(), step_records.begin(), step_records.end());
        }

        return output;
    }

    //Will generate a full month of data
    std::vector<SensorRecord> Simulator::generate_month(SimulatorProfile profile, int month){
        using namespace std::chrono;

        int days_in_month;
        if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12){
            days_in_month = 31;
        }else if (month == 4 || month == 6 || month == 9 || month == 11){
            days_in_month = 30;
        }else{
            days_in_month = 28;
        }
        
        std::vector<SensorRecord> output;
        output.reserve(days_in_month * 13000);

        const int year = 2024;
        for (int day = 1; day <= days_in_month; ++day){
            std::tm local{};
            local.tm_year = year - 1900;
            local.tm_mon  = month - 1;
            local.tm_mday = day;
            local.tm_hour = 0;

            std::time_t tt = timegm(&local);
            system_clock::time_point midnight = system_clock::from_time_t(tt);

            clock_ = Clock(midnight, 60);
            last_pm25_glebe_     = 20.0;
            last_pm25_downtown_  = 20.0;
            last_pm25_byward_    = 20.0;

            last_noise_glebe_    = 50.0;
            last_noise_downtown_ = 50.0;
            last_noise_byward_   = 50.0;

            auto records = generate_day(profile);
            output.insert(output.end(), records.begin(), records.end());
    }
    return output;
}

    //Generates 12 months of data
    std::vector<SensorRecord> Simulator::generate_year(SimulatorProfile profile){
        std::vector<SensorRecord> output;
        output.reserve(12 * 403000);

        for (int month = 1; month <= 12; month++){

            auto month_record = generate_month(profile, month);
            output.insert(output.end(),
              std::make_move_iterator(month_record.begin()),
              std::make_move_iterator(month_record.end()));
        }
        return output;
    }
    
    

    void write_csv_files(const std::vector<SensorRecord>& records, const std::string& traffic_file, const std::string& air_file, const std::string& noise_file){
        std::ofstream traffic_out(traffic_file);
        std::ofstream air_out(air_file);
        std::ofstream noise_out(noise_file);

        if (!traffic_out.is_open() || !air_out.is_open() || !noise_out.is_open())
            throw std::runtime_error("Failed to open one or more CSV output files.");

        // Write headers
        traffic_out << "timestamp,sensor_id,zone_id,speed,flow\n";
        air_out     << "timestamp,sensor_id,zone_id,pm25,pm10\n";
        noise_out   << "timestamp,sensor_id,zone_id,db\n";

        for (const auto& r : records)
        {

            if (r.speed.has_value() || r.flow.has_value())
            {
                traffic_out
                    << r.ts << ","
                    << r.sensor_id << ","
                    << r.zone_id << ","
                    << (r.speed.has_value() ? std::to_string(*r.speed) : "") << ","
                    << (r.flow.has_value()  ? std::to_string(*r.flow)  : "")
                    << "\n";
            }

            if (r.pm25.has_value() || r.pm10.has_value())
            {
                air_out
                    << r.ts << ","
                    << r.sensor_id << ","
                    << r.zone_id << ","
                    << (r.pm25.has_value() ? std::to_string(*r.pm25) : "") << ","
                    << (r.pm10.has_value() ? std::to_string(*r.pm10) : "")
                    << "\n";
            }

            if (r.db.has_value())
            {
                noise_out
                    << r.ts << ","
                    << r.sensor_id << ","
                    << r.zone_id << ","
                    << *r.db
                    << "\n";
            }
        }
    }

    //Generates traffic data, values will increase/decrease in 'random' increments (deterministically) based on the previous value
    SensorRecord Simulator::generate_traffic_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, SimulatorProfile profile){
        SensorRecord output{};
        output.ts = ts;
        output.zone_id = zone_id;
        output.sensor_id = sensor_id;
        
        int hour = hour_of_day(ts);
        bool is_weekday = (profile == SimulatorProfile::Weekday);

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
            double rush_slowdown_min = -10;
            double rush_slowdown_max = 10;
            if (hour >= 7 && hour <= 9) {  // AM rush         
                switch(zone_id){
                    // Downtown rush hour slows traffic more than Byward and Glebe, Byward slows less than Downtown but more than Glebe, etc
                    case 1: rush_slowdown_min = -25; rush_slowdown_max = -12; break;
                    case 2: rush_slowdown_min = -20; rush_slowdown_max = -8; break; 
                    case 3: rush_slowdown_min = -17; rush_slowdown_max = -8;  break; 
                }
            } else if (hour >= 16 && hour <= 18) {       // PM rush
                switch(zone_id){
                    // Downtown rush hour slows traffic more than Byward and Glebe, Byward slows less than Downtown but more than Glebe, etc
                    case 1: rush_slowdown_min = -25; rush_slowdown_max = -12; break;
                    case 2: rush_slowdown_min = -20; rush_slowdown_max = -8; break; 
                    case 3: rush_slowdown_min = -17; rush_slowdown_max = -8;  break; 
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

    //Generates air data, values will increase/decrease in 'random' increments (deterministically) based on the previous value
    SensorRecord Simulator::generate_air_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, SimulatorProfile profile){
        (void)profile;
        SensorRecord output{};
        output.ts = ts;
        output.zone_id = zone_id;
        output.sensor_id = sensor_id;

        int hour = hour_of_day(ts);

        double min_step = -1.9;
        double max_step =  1.5;

        if (hour >= 7 && hour <= 9) {
            min_step = -1.4;
            max_step = +1.8;
        }

        else if (hour >= 10 && hour <= 16) {
            min_step = -3.9;
            max_step = +0.3;
        }

        else if (hour >= 1 && hour <= 5) {
            min_step = -2.9;
            max_step = +0.3;
        }
        
        double& pmref = (zone_id == 1) ? last_pm25_downtown_ :
                        (zone_id == 2) ? last_pm25_glebe_ :
                                         last_pm25_byward_;


        double delta = rng_.uniform(min_step, max_step);
        pmref += delta;

        pmref = std::clamp(pmref, 5.0, 75.0);

        double pm10 = pmref * rng_.uniform(1.3, 1.8);

        output.pm25 = pmref;
        output.pm10 = pm10;

        return output;
    }


    //Generates noise data, values will increase/decrease in 'random' increments (deterministically) based on the previous value
    SensorRecord Simulator::generate_noise_record(std::chrono::system_clock::time_point ts, int zone_id, const std::string& sensor_id, SimulatorProfile profile){
        SensorRecord output{};
        output.ts = ts;
        output.sensor_id = sensor_id;
        output.zone_id = zone_id;

        int hour = hour_of_day(ts);
        double min_step = -2.75;
        double max_step = 2.5;

        if (hour >= 7 && hour <= 9) {
            min_step = -1.0;
            max_step =  5.0;      // weekday morning spike
        }

        if (profile == SimulatorProfile::Weekend && hour >= 20 && hour <= 23) {
            min_step =  0.0;
            max_step =  7.0;     // nightlife effect
        }

        double& dbref = (zone_id == 1) ? last_noise_downtown_ :
                        (zone_id == 2) ? last_noise_glebe_ :
                                         last_noise_byward_;

        double delta = rng_.uniform(min_step, max_step);
        dbref += delta;
        dbref = std::clamp(dbref, 35.0, 95.0);
        output.db = dbref;
        return output;
    }
}//namespace sim