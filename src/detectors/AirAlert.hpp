#pragma once
#include <vector>
#include <unordered_map>
#include <string>

#include "../core/Detector.hpp"
#include "../core/Window.hpp"
#include "../core/Finding.hpp"

namespace detectors {

    class AirAlert : public core::Detector {
        double pm25_thr_;

    public:
        explicit AirAlert(double pm25_thr) : pm25_thr_(pm25_thr) {}

        std::vector<core::Finding> detect(const core::Window& w) override {
            std::vector<core::Finding> findings;
            if (w.records.empty()) return findings;

            double sum = 0.0;
            int    count = 0;

            for (const auto& rec : w.records) {
                if (rec.pm25.has_value()) {
                    sum += *rec.pm25;
                    ++count;
                }
            }
            if (count == 0) return findings;

            const double rolling_mean = sum / static_cast<double>(count);
            if (rolling_mean > pm25_thr_) {
                std::unordered_map<std::string, double> thresholds;
                thresholds["pm25_thr"] = pm25_thr_;

                const auto start = w.records.front().ts;
                const auto end = w.records.back().ts;

                core::Finding f{
                    "AirAlert",
                    rolling_mean,
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
