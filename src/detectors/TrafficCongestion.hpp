#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../core/Detector.hpp"
#include "../core/Window.hpp"
#include "../model/SensorRecord.hpp"
#include "Finding.hpp"

namespace detectors {

    class TrafficCongestion : public core::Detector {
    public:
        // Defaults: congestion if avg speed < 30 km/h for >= 3 consecutive minutes.
        TrafficCongestion(double speed_threshold_kmh = 30.0,
            int min_consecutive_minutes = 3)
            : speed_threshold_kmh_{ speed_threshold_kmh }
            , min_consecutive_minutes_{ min_consecutive_minutes }
        {
        }

        // core::Detector interface: recompute and store findings.
        void detect(const core::Window& window) override {
            latest_ = run(window);
        }

        // Direct API returning fresh findings for a given window.
        [[nodiscard]] std::vector<Finding> run(const core::Window& window) const {
            using namespace std::chrono;

            // zone -> minute_bucket -> {sum_speed, count}
            struct BucketStats {
                double sum_speed{ 0.0 };
                int count{ 0 };
            };

            std::unordered_map<int, std::unordered_map<std::int64_t, BucketStats>> by_zone;

            for (const auto& rec : window.records) {
                if (!rec.speed) continue;

                const auto epoch_seconds =
                    duration_cast<seconds>(rec.ts.time_since_epoch()).count();
                const std::int64_t bucket_start =
                    (epoch_seconds / 60) * 60; // minute granularity

                auto& bucket = by_zone[rec.zone_id][bucket_start];
                bucket.sum_speed += *rec.speed;
                bucket.count += 1;
            }

            std::vector<Finding> findings;

            for (auto& [zone_id, buckets] : by_zone) {
                if (buckets.empty()) continue;

                // Turn into sorted list of (bucket_start, avg_speed)
                std::vector<std::pair<std::int64_t, double>> series;
                series.reserve(buckets.size());

                for (const auto& [ts, stats] : buckets) {
                    if (stats.count == 0) continue;
                    series.emplace_back(ts, stats.sum_speed / stats.count);
                }

                if (series.empty()) continue;

                std::sort(series.begin(), series.end(),
                    [](const auto& a, const auto& b) {
                        return a.first < b.first;
                    });

                int run_len = 0;
                std::int64_t prev_bucket = 0;
                double last_slow_speed = 0.0;

                for (const auto& [bucket_start, avg_speed] : series) {
                    const bool slow = (avg_speed < speed_threshold_kmh_);
                    if (!slow) {
                        run_len = 0;
                        continue;
                    }

                    if (run_len == 0) {
                        run_len = 1;
                    }
                    else if (bucket_start == prev_bucket + 60) {
                        ++run_len;
                    }
                    else {
                        run_len = 1;
                    }

                    prev_bucket = bucket_start;
                    last_slow_speed = avg_speed;

                    if (run_len >= min_consecutive_minutes_) {
                        Finding f;
                        f.detector = "TrafficCongestion";
                        f.zone_id = zone_id;
                        f.metric_value = last_slow_speed;
                        f.description =
                            "Traffic congestion: avg speed below " +
                            std::to_string(speed_threshold_kmh_) +
                            " km/h for at least " +
                            std::to_string(min_consecutive_minutes_) +
                            " consecutive minutes";
                        findings.push_back(std::move(f));
                        break; // one finding per zone is enough
                    }
                }
            }

            return findings;
        }

        [[nodiscard]] const std::vector<Finding>& latest_findings() const {
            return latest_;
        }

    private:
        double speed_threshold_kmh_;
        int min_consecutive_minutes_;
        std::vector<Finding> latest_;
    };

} // namespace detectors
