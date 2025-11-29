#pragma once
#include "../core/Detector.hpp"
#include "../core/Window.hpp"
#include "../core/Finding.hpp"

namespace detectors {

class AirAlert : public core::Detector {
    double pm25_thr_;
public:
    explicit AirAlert(double pm25_thr) : pm25_thr_(pm25_thr) {}

    std::vector<core::Finding> detect(const core::Window& w) override {
        // TODO: rolling mean PM2.5 above threshold.
        //(void)pm25_thr_;
        std::vector<core::Finding> findings;
        double numerator = 0.0;
        int denominator = 0;

        for (const auto& rec : w.records) {
            if (rec.pm25.has_value()) { numerator += rec.pm25.value(); denominator++; }
        }
        double rolling_mean = numerator / denominator;
        
        if (rolling_mean > pm25_thr_) {
            std::unordered_map<std::string,double> thresholds;
            thresholds["pm25_thr"] = pm25_thr_;

            std::chrono::system_clock::time_point start = w.records.front().ts;
            std::chrono::system_clock::time_point end = w.records.back().ts;

            core::Finding finding{"AirAlert",rolling_mean,thresholds,start,end};
            findings.push_back(finding);
        }
        return findings;
    }
};

} // namespace detectors
