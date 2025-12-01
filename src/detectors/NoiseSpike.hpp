#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <chrono>

#include "../core/Detector.hpp"
#include "../core/Window.hpp"
#include "../core/Finding.hpp"

namespace detectors {

    class NoiseSpike : public core::Detector {
        double db_thr_;
        int    count_thr_;
        int    window_min_;

    public:
        NoiseSpike(double db_thr, int count_thr, int window_min)
            : db_thr_(db_thr), count_thr_(count_thr), window_min_(window_min) {
        }

        std::vector<core::Finding> detect(const core::Window& w) override {
            std::vector<core::Finding> findings;
            if (w.records.empty()) return findings;

            core::Window timed_win = w; // local copy we can trim

            const auto max_ts = timed_win.records.back().ts;
            const auto cutoff = max_ts - std::chrono::minutes(window_min_);

            auto erase_it = std::remove_if(
                timed_win.records.begin(),
                timed_win.records.end(),
                [&](const model::SensorRecord& rec) {
                    return rec.ts < cutoff;
                }
            );
            timed_win.records.erase(erase_it, timed_win.records.end());
            if (timed_win.records.empty()) return findings;

            int  spike_count = 0;
            bool exceed_count_thr = false;

            for (const auto& rec : timed_win.records) {
                if (rec.db.has_value() && rec.db.value() > db_thr_) {
                    ++spike_count;
                    if (spike_count >= count_thr_) {
                        exceed_count_thr = true;
                        break;
                    }
                }
            }

            if (exceed_count_thr) {
                std::unordered_map<std::string, double> thresholds;
                thresholds["db_thr"] = db_thr_;
                thresholds["count_thr"] = static_cast<double>(count_thr_);
                thresholds["window_min"] = static_cast<double>(window_min_);

                const auto start = timed_win.records.front().ts;
                const auto end = timed_win.records.back().ts;

                core::Finding f{
                    "NoiseSpike",
                    static_cast<double>(spike_count),
                    std::move(thresholds),
                    start,
                    end
                };
                findings.push_back(std::move(f));
            }

            return findings;
        }
    };

} // namespace detectors
