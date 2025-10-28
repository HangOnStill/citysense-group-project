#pragma once
#include "../core/Detector.hpp"

namespace detectors {

class AirAlert : public core::Detector {
    double pm25_thr_;
public:
    explicit AirAlert(double pm25_thr) : pm25_thr_(pm25_thr) {}

    void detect(const core::Window& /*w*/) override {
        // TODO: rolling mean PM2.5 above threshold.
        (void)pm25_thr_;
    }
};

} // namespace detectors
