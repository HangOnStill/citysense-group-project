#pragma once
#include "../core/Detector.hpp"
#include "../core/Window.hpp"
#include "../core/Finding.hpp"
#include <map>

namespace detectors {
    using timestamp = std::chrono::system_clock::time_point;

class TrafficCongestion : public core::Detector {
    double speed_thr_;
    int consec_min_;
public:
    TrafficCongestion(double speed_thr, int consec_min)
      : speed_thr_(speed_thr), consec_min_(consec_min) {}

    std::vector<core::Finding> detect(const core::Window& w) override {
        // TODO: compute consecutive minutes with avg speed below threshold.
        //(void)speed_thr_; (void)consec_min_;
        std::vector<core::Finding> findings;
        std::unordered_map<std::string,double> thresholds;
        thresholds["speed_thr"] = speed_thr_;
        thresholds["consec_min_"] = consec_min_;

        std::map<timestamp,double> speeds_per_min = group_recs_by_min(w);

        int current_streak = 0;

        auto current_start = speeds_per_min.begin();
        auto current_end = speeds_per_min.begin();

        for (const auto& [time,avg_speed] : speeds_per_min) {
            if (avg_speed <= speed_thr_) { current_streak++; current_end++; }

            else if (current_streak > consec_min_) {
                if (current_end == speeds_per_min.end()) --current_end;

                timestamp start = current_start->first;
                timestamp end = current_end->first;

                core::Finding finding{"TrafficCongestion",current_streak,thresholds,start,end};
                findings.push_back(finding);
                
                current_streak = 0; current_end++; current_start = current_end; 
            }
            else { 
                current_streak = 0; current_end++; current_start = current_end; 
            }
        }
        return findings;
    }

    std::map<timestamp, double> group_recs_by_min(const core::Window& w) {
        std::map<timestamp, double> speeds_per_min;
        std::map<timestamp, int> recs_per_min;

        for (const auto& rec : w.records) {
            if (rec.speed.has_value()) {
                timestamp floor_ts = std::chrono::floor<std::chrono::minutes>(rec.ts);

                speeds_per_min[floor_ts] += rec.speed.value();
                recs_per_min[floor_ts]++;
            }
        }
        for (const auto& [time,speed] : speeds_per_min) {
            int denominator = recs_per_min[time];

            speeds_per_min[time] /= denominator;
        }
        return speeds_per_min;
    }
};

} // namespace detectors
