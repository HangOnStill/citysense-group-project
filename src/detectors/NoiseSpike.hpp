#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../core/Detector.hpp"
#include "../core/Window.hpp"
#include "../model/SensorRecord.hpp"
#include "Finding.hpp"

namespace detectors {

    class NoiseSpike : public core::Detector {
    public:
        // Defaults: spike if dB >= 80 at least 3 times in the current window.
        NoiseSpike(double db_threshold = 80.0,
            int min_events = 3)
            : db_threshold_{ db_threshold }
            , min_events_{ min_events }
        {
        }

        void detect(const core::Window& window) override {
            latest_ = run(window);
        }

        [[nodiscard]] std::vector<Finding> run(const core::Window& window) const {
            std::unordered_map<int, int> counts_by_zone;

            for (const auto& rec : window.records) {
                if (!rec.db) continue;
                if (*rec.db >= db_threshold_) {
                    counts_by_zone[rec.zone_id] += 1;
                }
            }

            std::vector<Finding> findings;

            for (const auto& [zone_id, count] : counts_by_zone) {
                if (count < min_events_) continue;

                Finding f;
                f.detector = "NoiseSpike";
                f.zone_id = zone_id;
                f.metric_value = static_cast<double>(count);
                f.description =
                    "Noise spike: dB >= " + std::to_string(db_threshold_) +
                    " occurred at least " + std::to_string(min_events_) +
                    " times in the current window";
                findings.push_back(std::move(f));
            }

            return findings;
        }

        [[nodiscard]] const std::vector<Finding>& latest_findings() const {
            return latest_;
        }

    private:
        double db_threshold_;
        int min_events_;
        std::vector<Finding> latest_;
    };

} // namespace detectors
