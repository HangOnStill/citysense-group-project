#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../core/Detector.hpp"
#include "../core/Window.hpp"
#include "../model/SensorRecord.hpp"
#include "Finding.hpp"

namespace detectors {

    class AirAlert : public core::Detector {
    public:
        // Defaults: alert if mean PM2.5 > 35 µg/m³ with at least 5 samples.
        AirAlert(double pm25_threshold = 35.0,
            int min_samples = 5)
            : pm25_threshold_{ pm25_threshold }
            , min_samples_{ min_samples }
        {
        }

        void detect(const core::Window& window) override {
            latest_ = run(window);
        }

        [[nodiscard]] std::vector<Finding> run(const core::Window& window) const {
            struct Stats {
                double sum{ 0.0 };
                int count{ 0 };
            };

            std::unordered_map<int, Stats> by_zone;

            for (const auto& rec : window.records) {
                if (!rec.pm25) continue;
                auto& s = by_zone[rec.zone_id];
                s.sum += *rec.pm25;
                s.count += 1;
            }

            std::vector<Finding> findings;

            for (const auto& [zone_id, stats] : by_zone) {
                if (stats.count < min_samples_) continue;

                const double mean = stats.sum / stats.count;
                if (mean > pm25_threshold_) {
                    Finding f;
                    f.detector = "AirAlert";
                    f.zone_id = zone_id;
                    f.metric_value = mean;
                    f.description =
                        "Air-quality alert: mean PM2.5 above " +
                        std::to_string(pm25_threshold_) +
                        " µg/m³ (avg=" +
                        std::to_string(mean) + ")";
                    findings.push_back(std::move(f));
                }
            }

            return findings;
        }

        [[nodiscard]] const std::vector<Finding>& latest_findings() const {
            return latest_;
        }

    private:
        double pm25_threshold_;
        int min_samples_;
        std::vector<Finding> latest_;
    };

} // namespace detectors
