#pragma once
#include "../core/Detector.hpp"
#include "../core/Window.hpp"
#include "../core/Finding.hpp"

namespace detectors {

class NoiseSpike : public core::Detector {
    double db_thr_;
    int count_thr_;
    int window_min_;
public:
    NoiseSpike(double db_thr, int count_thr, int window_min)
      : db_thr_(db_thr), count_thr_(count_thr), window_min_(window_min) {}

    std::vector<core::Finding> detect(const core::Window& w) override {
        // TODO: db threshold exceeded M times in T minutes.
        //(void)db_thr_; (void)count_thr_; (void)window_min_;
        std::vector<core::Finding> findings;
        
        core::Window timed_win = w;
        const auto max_ts = timed_win.records.back().ts;
        const auto cutoff = max_ts - std::chrono::minutes(window_min_);

        // NOTE: We are only considering records in the last window_min_ minutes
        auto erase_it = std::remove_if(
            timed_win.records.begin(),
            timed_win.records.end(),
        [&](const model::SensorRecord& rec){ return rec.ts < cutoff; });

        timed_win.records.erase(erase_it, timed_win.records.end());

        int spike_count;
        bool exceed_count_thr = false;
        for (const auto& rec : timed_win.records) {
            if (rec.db.has_value() && rec.db.value() > db_thr_) {
                ++spike_count;
            }
            if (spike_count > count_thr_) exceed_count_thr = true;
        }

        if (exceed_count_thr) {
            std::unordered_map<std::string,double> thresholds;
            thresholds["db_thr"] = db_thr_;
            thresholds["count_thr"] = count_thr_;
            thresholds["window_min"] = window_min_;

            std::chrono::system_clock::time_point start = timed_win.records.front().ts;
            std::chrono::system_clock::time_point end = timed_win.records.back().ts;

            core::Finding finding{"NoiseSpike",spike_count,thresholds,start,end};
            findings.push_back(finding);
        }
        return findings;
    }
};

} // namespace detectors
