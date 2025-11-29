#include "PatternAnalytics.hpp"
#include <chrono>
#include <ctime>
#include <iostream>

namespace sim {

    using model::SensorRecord;

    static int hour_of(const std::chrono::system_clock::time_point& ts) {
        using namespace std::chrono;
        long sec = duration_cast<seconds>(ts.time_since_epoch()).count();
        return static_cast<int>((sec / 3600) % 24);
    }


    DailyFlags analyse_day(const std::vector<SensorRecord>& day, const Criteria& criteria){
        DailyFlags flags{};
        if (day.empty()) return flags;

        //Used to store consecutive minutes of data above threshold, while keeping each zone seperate
        int pm25_run_glebe = 0;
        int pm25_run_downtown = 0;
        int pm25_run_byward = 0;

        int noise_run_glebe = 0;
        int noise_run_downtown = 0;
        int noise_run_byward = 0;

        bool pm25_flagged = false;
        bool noise_flagged = false;
        bool traffic_flag = false;

        //Traffic window accumulators
        double am_speed_sum = 0.0;
        int    am_speed_count = 0;

        double pm_speed_sum = 0.0;
        int    pm_speed_count = 0;


        for (const auto& rec : day) {

            int h = hour_of(rec.ts);

            //Air Analysis
            if (rec.pm25.has_value()) {
                if (rec.zone_id == 1){
                    if (*rec.pm25 > criteria.pm25_threshold) {
                        pm25_run_downtown++;
                        if (pm25_run_downtown >= criteria.pm25_mins_required){
                            pm25_flagged = true;
                        }
                    } else {
                        pm25_run_downtown = 0;
                    }
                }else if (rec.zone_id == 2){
                    if (*rec.pm25 > criteria.pm25_threshold) {
                        pm25_run_glebe++;
                        if (pm25_run_glebe >= criteria.pm25_mins_required){
                            pm25_flagged = true;
                        }
                    } else {
                        pm25_run_glebe = 0;
                    }
                }else{
                    if (*rec.pm25 > criteria.pm25_threshold) {
                        pm25_run_byward++;
                        if (pm25_run_byward >= criteria.pm25_mins_required){
                            pm25_flagged = true;
                        }
                    } else {
                        pm25_run_byward = 0;
                    }
                }
            }   



            //Noise Analysis
            if (rec.db.has_value()) {
                if (h >= 18 && h <= 22){
                    if (rec.zone_id == 1){
                        if (*rec.db > criteria.noise_threshold) {
                            noise_run_downtown++;
                            if (noise_run_downtown >= criteria.noise_mins_required){
                                noise_flagged = true;
                            }
                        } else {
                            noise_run_downtown = 0;
                        }
                    }else if (rec.zone_id == 2){
                        if (*rec.db > criteria.noise_threshold) {
                            noise_run_glebe++;
                            if (noise_run_glebe >= criteria.noise_mins_required){
                                noise_flagged = true;
                            }
                        } else {
                            noise_run_glebe = 0;
                        }
                    }else{
                        if (*rec.db > criteria.noise_threshold) {
                            noise_run_byward++;
                            if (noise_run_byward >= criteria.noise_mins_required){
                                noise_flagged = true;
                            }
                        } else {
                            noise_run_byward = 0;
                        }
                    }
                }
            }

            //Traffic Analysis
            if (rec.speed) {
                double s = *rec.speed;

                //AM rush
                if (h >= 7 && h <= 9) {
                    am_speed_sum += s;
                    am_speed_count++;
                }

                //PM rush
                if (h >= 16 && h <= 18) {
                    pm_speed_sum += s;
                    pm_speed_count++;
                }
            }
        }

        bool am_fail = false;
        bool pm_fail = false;

        if (am_speed_count > 0)
            am_fail = (am_speed_sum / am_speed_count) < criteria.rush_speed_threshold;

        if (pm_speed_count > 0)
            pm_fail = (pm_speed_sum / pm_speed_count) < criteria.rush_speed_threshold;

        traffic_flag = (am_fail && pm_fail);

        //Store whether this day has exceeded thresholds
        flags.air_flag     = pm25_flagged;
        flags.noise_flag   = noise_flagged;
        flags.traffic_flag = traffic_flag;

        return flags;
    }

    //Helper to get day of month out of record
    static int extract_day(const SensorRecord& r) {
        using namespace std::chrono;
        long sec = duration_cast<seconds>(r.ts.time_since_epoch()).count();
        std::time_t t = sec;
        std::tm* tm = std::gmtime(&t);
        return tm->tm_mday;
    }

    //Helper to get month of year out of record
    static int extract_month(const SensorRecord& r) {
        using namespace std::chrono;
        long sec = duration_cast<seconds>(r.ts.time_since_epoch()).count();
        std::time_t t = sec;
        std::tm* tm = std::gmtime(&t);
        return tm->tm_mon + 1;
    }

    //Given a months worth of records, extracts all flags raised over each day, summing the total and storing them in a ADT
    MonthlySummary analyse_month(const std::vector<SensorRecord>& month, const Criteria& criteria){
        MonthlySummary ms{};
        if (month.empty()) return ms;

        std::vector<SensorRecord> current_day;
        int current_day_num = extract_day(month.front());
        int current_month_num = extract_month(month.front());

        for (const auto& rec : month) {
            int d = extract_day(rec);

            if (d != current_day_num) {
                // finish previous day
                DailyFlags df = analyse_day(current_day, criteria);
                if (df.air_flag)          ms.air_days++;
                if (df.traffic_flag) ms.traffic_days++;
                if (df.noise_flag)        ms.noise_days++;

                current_day.clear();
                current_day_num = d;
            }

            current_day.push_back(rec);
        }

        // Last day
        if (!current_day.empty()) {
            DailyFlags df = analyse_day(current_day, criteria);
            if (df.air_flag)          ms.air_days++;
            if (df.traffic_flag) ms.traffic_days++;
            if (df.noise_flag)        ms.noise_days++;
        }
        ms.month_num = current_month_num;
        return ms;
    }
    

    //Given a years worth of data, extracts each month, then calls extract_month (above function) on each
    YearlySummary analyse_year(const std::vector<SensorRecord>& year,
                            const Criteria& criteria)
    {
        YearlySummary ys{};
        if (year.empty()) return ys;

        ys.months.reserve(12);

        std::vector<SensorRecord> current_month;
        int current_month_num = extract_month(year.front());

        for (const auto& rec : year) {
            int m = extract_month(rec);

            if (m != current_month_num) {
                ys.months.push_back(analyse_month(current_month, criteria));
                current_month.clear();
                current_month_num = m;
            }

            current_month.push_back(rec);
        }

        if (!current_month.empty()) {
            ys.months.push_back(analyse_month(current_month, criteria));
        }

        return ys;
    }

    //Used to help print output
    static const char* MONTH_NAMES[12] = {
        "January","February","March","April","May","June",
        "July","August","September","October","November","December"
    };

    //Given the analysed data from a month, outputs how many of each flag were raised
    void print_month_summary(const sim::MonthlySummary& ms){
            std::cout << MONTH_NAMES[ms.month_num - 1] << ":\n";
            std::cout << "  Air flags:     " << ms.air_days << "\n";
            std::cout << "  Traffic flags: " << ms.traffic_days << "\n";
            std::cout << "  Noise flags:   " << ms.noise_days << "\n\n";
    }

    //Given the analysed data for a year, calls the above function on each month
    void print_year_summary(const sim::YearlySummary& ys) {
        for (int i = 0; i < ys.months.size(); i++) {
            const auto& m = ys.months[i];
            std::cout << MONTH_NAMES[i] << ":\n";
            std::cout << "  Air flags:     " << m.air_days << "\n";
            std::cout << "  Traffic flags: " << m.traffic_days << "\n";
            std::cout << "  Noise flags:   " << m.noise_days << "\n\n";
        }
    }

} // namespace sim
