#include <chrono>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>

#include "Simulator.hpp"

namespace sim {
    using model::SensorRecord;

    namespace {
        inline std::time_t timegm_utc(std::tm* tm) {
#if defined(_WIN32)
            return _mkgmtime(tm);   // MSVC / Windows
#else
            return timegm(tm);      // POSIX
#endif
        }

        // Helper: hour of day (0–23) from system_clock::time_point
        inline int hour_of_day(std::chrono::system_clock::time_point ts) {
            using namespace std::chrono;
            auto secs = duration_cast<seconds>(ts.time_since_epoch()).count(); // typically long long
            auto hrs = secs / 3600;
            int h = static_cast<int>(hrs % 24);
            return (h < 0) ? h + 24 : h;
        }
    }

    Simulator::Simulator(Clock clock, SeededRNG rng)
        : clock_(clock),
        rng_(rng) {
    }

    void Simulator::start(SimulatorProfile profile) {
        profile_ = profile;
        running_ = true;

        last_pm25_glebe_ = 20.0;
        last_pm25_downtown_ = 20.0;
        last_pm25_byward_ = 20.0;

        last_noise_glebe_ = 50.0;
        last_noise_downtown_ = 50.0;
        last_noise_byward_ = 50.0;
    }

    void Simulator::pause() {
        running_ = false;
    }

    void Simulator::resume() {
        running_ = true;
    }

    std::vector<SensorRecord> Simulator::generate_step(SimulatorProfile profile) {
        using namespace std::chrono;

        std::vector<SensorRecord> output;
        output.reserve(9);

        auto ts = clock_.now();

        const std::vector<std::string> traffic_sensors = {
            "traffic-glebe", "traffic-downtown", "traffic-byward"
        };
        const std::vector<std::string> air_sensors = {
            "air-glebe", "air-downtown", "air-byward"
        };
        const std::vector<std::string> noise_sensors = {
            "noise-glebe", "noise-downtown", "noise-byward"
        };

        constexpr int ZONE_DOWNTOWN = 1;
        constexpr int ZONE_GLEBE = 2;
        constexpr int ZONE_BYWARD = 3;

        // Traffic
        output.push_back(generate_traffic_record(ts, ZONE_GLEBE, traffic_sensors[0], profile));
        output.push_back(generate_traffic_record(ts, ZONE_DOWNTOWN, traffic_sensors[1], profile));
        output.push_back(generate_traffic_record(ts, ZONE_BYWARD, traffic_sensors[2], profile));

        // Air
        output.push_back(generate_air_record(ts, ZONE_GLEBE, air_sensors[0], profile));
        output.push_back(generate_air_record(ts, ZONE_DOWNTOWN, air_sensors[1], profile));
        output.push_back(generate_air_record(ts, ZONE_BYWARD, air_sensors[2], profile));

        // Noise
        output.push_back(generate_noise_record(ts, ZONE_GLEBE, noise_sensors[0], profile));
        output.push_back(generate_noise_record(ts, ZONE_DOWNTOWN, noise_sensors[1], profile));
        output.push_back(generate_noise_record(ts, ZONE_BYWARD, noise_sensors[2], profile));

        clock_.advance();
        return output;
    }

    std::vector<SensorRecord> Simulator::next_step() {
        if (!running_) {
            return {};
        }
        return generate_step(profile_);
    }

    std::vector<SensorRecord> Simulator::next_batch(int steps) {
        std::vector<SensorRecord> output;
        output.reserve(steps * 9);

        for (int i = 0; i < steps; ++i) {
            auto step = next_step();
            if (step.empty()) break;
            output.insert(output.end(), step.begin(), step.end());
        }
        return output;
    }

    std::vector<SensorRecord> Simulator::generate_day(SimulatorProfile profile) {
        using namespace std::chrono;

        std::vector<SensorRecord> output;
        output.reserve(9 * 1440);

        const auto start_time = clock_.now();
        const auto end_time = start_time + hours(24);

        while (clock_.now() < end_time) {
            auto step_records = generate_step(profile);
            output.insert(output.end(), step_records.begin(), step_records.end());
        }

        return output;
    }

    std::vector<SensorRecord> Simulator::generate_month(SimulatorProfile profile, int month) {
        using namespace std::chrono;

        int days_in_month;
        if (month == 1 || month == 3 || month == 5 || month == 7 ||
            month == 8 || month == 10 || month == 12) {
            days_in_month = 31;
        }
        else if (month == 4 || month == 6 || month == 9 || month == 11) {
            days_in_month = 30;
        }
        else {
            days_in_month = 28;
        }

        std::vector<SensorRecord> output;
        output.reserve(days_in_month * 13000);

        const int year = 2024;
        for (int day = 1; day <= days_in_month; ++day) {
            std::tm local{};
            local.tm_year = year - 1900;
            local.tm_mon = month - 1;
            local.tm_mday = day;
            local.tm_hour = 0;

            std::time_t tt = timegm_utc(&local);
            auto midnight = std::chrono::system_clock::from_time_t(tt);

            clock_ = Clock(midnight, 60);

            last_pm25_glebe_ = 20.0;
            last_pm25_downtown_ = 20.0;
            last_pm25_byward_ = 20.0;

            last_noise_glebe_ = 50.0;
            last_noise_downtown_ = 50.0;
            last_noise_byward_ = 50.0;

            auto records = generate_day(profile);
            output.insert(output.end(), records.begin(), records.end());
        }

        return output;
    }

    std::vector<SensorRecord> Simulator::generate_year(SimulatorProfile profile) {
        std::vector<SensorRecord> output;
        output.reserve(12 * 403000);

        for (int month = 1; month <= 12; ++month) {
            auto month_record = generate_month(profile, month);
            output.insert(
                output.end(),
                std::make_move_iterator(month_record.begin()),
                std::make_move_iterator(month_record.end())
            );
        }
        return output;
    }

    void write_csv_files(const std::vector<SensorRecord>& records,
        const std::string& traffic_file,
        const std::string& air_file,
        const std::string& noise_file) {
        std::ofstream traffic_out(traffic_file);
        std::ofstream air_out(air_file);
        std::ofstream noise_out(noise_file);

        if (!traffic_out.is_open() || !air_out.is_open() || !noise_out.is_open())
            throw std::runtime_error("Failed to open one or more CSV output files.");

        traffic_out << "timestamp,sensor_id,zone_id,speed,flow\n";
        air_out << "timestamp,sensor_id,zone_id,pm25,pm10\n";
        noise_out << "timestamp,sensor_id,zone_id,db\n";

        for (const auto& r : records) {
            if (r.speed.has_value() || r.flow.has_value()) {
                traffic_out
                    << r.ts << ","
                    << r.sensor_id << ","
                    << r.zone_id << ","
                    << (r.speed ? std::to_string(*r.speed) : "") << ","
                    << (r.flow ? std::to_string(*r.flow) : "")
                    << "\n";
            }

            if (r.pm25.has_value() || r.pm10.has_value()) {
                air_out
                    << r.ts << ","
                    << r.sensor_id << ","
                    << r.zone_id << ","
                    << (r.pm25 ? std::to_string(*r.pm25) : "") << ","
                    << (r.pm10 ? std::to_string(*r.pm10) : "")
                    << "\n";
            }

            if (r.db.has_value()) {
                noise_out
                    << r.ts << ","
                    << r.sensor_id << ","
                    << r.zone_id << ","
                    << *r.db
                    << "\n";
            }
        }
    }

    SensorRecord Simulator::generate_traffic_record(std::chrono::system_clock::time_point ts,
        int zone_id,
        const std::string& sensor_id,
        SimulatorProfile profile) {
        SensorRecord output{};
        output.ts = ts;
        output.zone_id = zone_id;
        output.sensor_id = sensor_id;

        int hour = hour_of_day(ts);
        bool is_weekday = (profile == SimulatorProfile::Weekday);

        double base_min;
        double base_max;

        switch (zone_id) {
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
            double rush_slowdown_min = -10.0;
            double rush_slowdown_max = 10.0;

            if (hour >= 7 && hour <= 9) {
                switch (zone_id) {
                case 1: rush_slowdown_min = -25.0; rush_slowdown_max = -12.0; break;
                case 2: rush_slowdown_min = -20.0; rush_slowdown_max = -8.0; break;
                case 3: rush_slowdown_min = -17.0; rush_slowdown_max = -8.0; break;
                }
            }
            else if (hour >= 16 && hour <= 18) {
                switch (zone_id) {
                case 1: rush_slowdown_min = -25.0; rush_slowdown_max = -12.0; break;
                case 2: rush_slowdown_min = -20.0; rush_slowdown_max = -8.0; break;
                case 3: rush_slowdown_min = -17.0; rush_slowdown_max = -8.0; break;
                }
            }

            speed += rng_.uniform(rush_slowdown_min, rush_slowdown_max);
        }
        else {
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
        if (speed < 20.0)      flow = rng_.uniform(20.0, 35.0);
        else if (speed < 35.0) flow = rng_.uniform(15.0, 25.0);
        else                   flow = rng_.uniform(8.0, 18.0);

        output.flow = flow;
        return output;
    }

    SensorRecord Simulator::generate_air_record(std::chrono::system_clock::time_point ts,
        int zone_id,
        const std::string& sensor_id,
        SimulatorProfile profile) {
        (void)profile;

        SensorRecord output{};
        output.ts = ts;
        output.zone_id = zone_id;
        output.sensor_id = sensor_id;

        int hour = hour_of_day(ts);

        double min_step = -1.9;
        double max_step = 1.5;

        if (hour >= 7 && hour <= 9) {
            min_step = -1.4;
            max_step = 1.8;
        }
        else if (hour >= 10 && hour <= 16) {
            min_step = -3.9;
            max_step = 0.3;
        }
        else if (hour >= 1 && hour <= 5) {
            min_step = -2.9;
            max_step = 0.3;
        }

        double& pmref = (zone_id == 1) ? last_pm25_downtown_
            : (zone_id == 2) ? last_pm25_glebe_
            : last_pm25_byward_;

        double delta = rng_.uniform(min_step, max_step);
        pmref += delta;
        pmref = std::clamp(pmref, 5.0, 75.0);

        double pm10 = pmref * rng_.uniform(1.3, 1.8);

        output.pm25 = pmref;
        output.pm10 = pm10;

        return output;
    }

    SensorRecord Simulator::generate_noise_record(std::chrono::system_clock::time_point ts,
        int zone_id,
        const std::string& sensor_id,
        SimulatorProfile profile) {
        SensorRecord output{};
        output.ts = ts;
        output.sensor_id = sensor_id;
        output.zone_id = zone_id;

        int hour = hour_of_day(ts);
        double min_step = -2.75;
        double max_step = 2.5;

        if (hour >= 7 && hour <= 9) {
            min_step = -1.0;
            max_step = 5.0;
        }

        if (profile == SimulatorProfile::Weekend && hour >= 20 && hour <= 23) {
            min_step = 0.0;
            max_step = 7.0;
        }

        double& dbref = (zone_id == 1) ? last_noise_downtown_
            : (zone_id == 2) ? last_noise_glebe_
            : last_noise_byward_;

        double delta = rng_.uniform(min_step, max_step);
        dbref += delta;
        dbref = std::clamp(dbref, 35.0, 95.0);

        output.db = dbref;
        return output;
    }

} // namespace sim
