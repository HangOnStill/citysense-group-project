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
        core::Window timed_win = w;
        const auto max_ts = timed_win.records.back().ts;
        const auto cutoff = max_ts - std::chrono::minutes(window_min_);

        // NOTE: We are only considering records in the last window_min_ minutes
        auto erase_it = std::remove_if(
            timed_win.records.begin(),
            timed_win.records.end(),
        [&](const model::SensorRecord& rec){ return rec.ts < cutoff; });

        timed_win.records.erase(erase_it, timed_win.records.end());

        
    }
};

} // namespace detectors
