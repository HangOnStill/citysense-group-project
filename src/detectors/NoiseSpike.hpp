#pragma once
#include "../core/Detector.hpp"

namespace detectors {

class NoiseSpike : public core::Detector {
    double db_thr_;
    int count_thr_;
    int window_min_;
public:
    NoiseSpike(double db_thr, int count_thr, int window_min)
      : db_thr_(db_thr), count_thr_(count_thr), window_min_(window_min) {}

    void detect(const core::Window& /*w*/) override {
        // TODO: db threshold exceeded M times in T minutes.
        (void)db_thr_; (void)count_thr_; (void)window_min_;
    }
};

} // namespace detectors
