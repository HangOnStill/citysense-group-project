#pragma once
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

#include "../core/Detector.hpp"
#include "../core/Window.hpp"
#include "../core/Finding.hpp"

namespace detectors {

    using timestamp = std::chrono::system_clock::time_point;

    class TrafficCongestion : public core::Detector {
        double speed_thr_;
        int    consec_min_;

    public:
        TrafficCongestion(double speed_thr, int consec_min)
            : speed_thr_(speed_thr), consec_min_(consec_min) {
        }

        std::vector<core::Finding> detect(const core::Window& w) override {
            std::vector<core::Finding> findings;
            if (w.records.empty()) return findings;

            auto speeds_per_min = group_recs_by_min(w);
            if (speeds_per_min.empty()) return findings;

            std::unordered_map<std::string, double> thresholds;
            thresholds["speed_thr"] = speed_thr_;
            thresholds["consec_min"] = static_cast<double>(consec_min_);

            int current_streak = 0;
            auto streak_start = speeds_per_min.begin();

            auto it = speeds_per_min.begin();
            for (; it != speeds_per_min.end(); ++it) {
                const auto& [time, avg_speed] = *it;
                if (avg_speed <= speed_thr_) {     // or < speed_thr_ if you want strict
                    if (current_streak == 0) {
                        streak_start = it;
                    }
                    ++current_streak;
                }
                else {
                    if (current_streak >= consec_min_) {
                        auto end_it = std::prev(it);
                        core::Finding f{
                            "TrafficCongestion",
                            static_cast<double>(current_streak),
                            thresholds,
                            streak_start->first,
                            end_it->first
                        };
                        findings.push_back(f);
                    }
                    current_streak = 0;
                }
            }

            // tail streak
            if (current_streak >= consec_min_) {
                auto end_it = std::prev(speeds_per_min.end());
                core::Finding f{
                    "TrafficCongestion",
                    static_cast<double>(current_streak),
                    thresholds,
                    streak_start->first,
                    end_it->first
                };
                findings.push_back(f);
            }

            return findings;
        }

    private:
        std::map<timestamp, double> group_recs_by_min(const core::Window& w) {
            std::map<timestamp, double> speeds_per_min;
            std::map<timestamp, int>    recs_per_min;

            for (const auto& rec : w.records) {
                if (rec.speed.has_value()) {
                    auto floor_ts =
                        std::chrono::floor<std::chrono::minutes>(rec.ts);
                    speeds_per_min[floor_ts] += *rec.speed;
                    recs_per_min[floor_ts] += 1;
                }
            }

            for (auto& [time, sum_speed] : speeds_per_min) {
                const int denom = recs_per_min[time];
                if (denom > 0) {
                    sum_speed /= static_cast<double>(denom);
                }
            }
            return speeds_per_min;
        }
    };

} // namespace detectors
