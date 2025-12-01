// src/core/HighNoiseDetector.hpp
#pragma once

#include <vector>
#include <limits>
#include <sstream>

#include "core/Detector.hpp"
#include "core/Window.hpp"
#include "core/Finding.hpp"
#include "model/SensorRecord.hpp"

namespace core {

    class HighNoiseDetector : public Detector {
    public:
        explicit HighNoiseDetector(double threshold_db = 80.0)
            : threshold_db_{ threshold_db } {
        }

        std::vector<Finding> detect(const Window& w) override {
            std::vector<Finding> out;

            if (w.records.empty()) {
                return out;
            }

            double max_db = -std::numeric_limits<double>::infinity();
            int    max_zone = 0;
            std::string max_sensor;
            std::chrono::system_clock::time_point max_ts{};

            // Scan all records in the window
            for (const auto& rec : w.records) {
                if (!rec.db.has_value()) {
                    continue;
                }

                double v = *rec.db;
                if (v > max_db) {
                    max_db = v;
                    max_zone = rec.zone_id;
                    max_sensor = rec.sensor_id;
                    max_ts = rec.ts;
                }
            }

            // No noise readings at all
            if (!std::isfinite(max_db)) {
                return out;
            }

            // Below threshold → no finding
            if (max_db < threshold_db_) {
                return out;
            }

            Finding f;
            f.detector = "HighNoiseDetector";
            f.type = "high-noise";

            {
                std::ostringstream oss;
                oss << "High noise: " << max_db << " dB (threshold " << threshold_db_ << " dB)";
                f.message = oss.str();
            }

            f.zone_id = max_zone;
            f.sensor_id = max_sensor;
            f.timestamp = max_ts;

            // Use helper methods we added to Window
            f.start_ts = w.start_ts();
            f.end_ts = w.end_ts();

            f.value = max_db;
            f.thresholds["db"] = threshold_db_;

            out.push_back(std::move(f));
            return out;
        }

    private:
        double threshold_db_;
    };

} // namespace core
