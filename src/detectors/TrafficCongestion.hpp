#pragma once
#include "../core/Detector.hpp"
#include "../core/Window.hpp"
#include "../core/Finding.hpp"

namespace detectors {

class TrafficCongestion : public core::Detector {
    double speed_thr_;
    int consec_min_;
public:
    TrafficCongestion(double speed_thr, int consec_min)
      : speed_thr_(speed_thr), consec_min_(consec_min) {}

    std::vector<core::Finding> detect(const core::Window& /*w*/) override {
        // TODO: compute consecutive minutes with avg speed below threshold.
        (void)speed_thr_; (void)consec_min_;
    }
};

} // namespace detectors
