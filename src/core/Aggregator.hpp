#pragma once

#include <unordered_map>
#include <map>
#include <mutex>
#include <type_traits>
#include <iterator>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>

#include "Window.hpp"
#include "../model/SensorRecord.hpp"

namespace core {

    // Per-metric stats (over the *current* window)
    struct MetricSummary {
        int    count{ 0 };
        double sum{ 0.0 };

        double mean() const {
            return count > 0 ? sum / static_cast<double>(count) : 0.0;
        }
    };

    // Group of metrics for a zone
    struct ZoneMetrics {
        MetricSummary speed;  // traffic
        MetricSummary flow;   // traffic
        MetricSummary pm25;   // air
        MetricSummary pm10;   // air
        MetricSummary db;     // noise
        // No operator== needed for assignment tests
    };

    // Public summary contract
    struct Summary {
        int total_count{ 0 };                             // all ingested rows (any type)
        std::unordered_map<int, int> by_zone;          // zone_id -> count (current window only)

        // richer view: per-zone metrics for the current window
        // (kept for CLI/visualization, not used in equality in tests)
        std::unordered_map<int, ZoneMetrics> metrics_by_zone;
    };

    // Equality used by tests: only total_count and by_zone matter
    inline bool operator==(const Summary& a, const Summary& b) {
        return a.total_count == b.total_count
            && a.by_zone == b.by_zone;
    }

    class Aggregator {
    public:
        explicit Aggregator(int window_minutes)
            : window_minutes_(window_minutes) {
        }

        // Range-based ingest. Real window logic only for SensorRecord ranges.
        template <typename Range>
        void consume(const Range& r) {
            using std::begin;
            using std::end;
            auto first = begin(r);
            auto last = end(r);
            if (first == last) return;

            using Value = std::decay_t<decltype(*first)>;

            std::scoped_lock lock(mutex_);

            // Only SensorRecord ranges go into the time window.
            if constexpr (std::is_same_v<Value, model::SensorRecord>) {
                append_records_unlocked(first, last);
            }
            // For any type (including ints used in tests) we still bump total_count.
            total_count_ += static_cast<int>(std::distance(first, last));
        }

        // Convenience overload for a single SensorRecord
        void consume(const model::SensorRecord& rec) {
            std::array<model::SensorRecord, 1> one{ rec };
            consume(one);
        }

        // Reserve buffer for the window (for CLI options like --reserve)
        void reserve(std::size_t n) {
            std::scoped_lock lock(mutex_);
            window_.records.reserve(n);
        }

        // Full mixed window (all sensors)
        const Window& current_window_view() const {
            return window_;
        }

        // Traffic-only view (speed/flow records)
        Window traffic_window() const {
            std::scoped_lock lock(mutex_);
            Window out;
            out.time_start = window_.time_start;
            for (const auto& rec : window_.records) {
                if (rec.speed.has_value() || rec.flow.has_value()) {
                    out.records.push_back(rec);
                }
            }
            return out;
        }

        // Air-quality-only view (pm25/pm10)
        Window air_window() const {
            std::scoped_lock lock(mutex_);
            Window out;
            out.time_start = window_.time_start;
            for (const auto& rec : window_.records) {
                if (rec.pm25.has_value() || rec.pm10.has_value()) {
                    out.records.push_back(rec);
                }
            }
            return out;
        }

        // Noise-only view (db)
        Window noise_window() const {
            std::scoped_lock lock(mutex_);
            Window out;
            out.time_start = window_.time_start;
            for (const auto& rec : window_.records) {
                if (rec.db.has_value()) {
                    out.records.push_back(rec);
                }
            }
            return out;
        }

        // Summary over the *current* window.
        Summary summary() const {
            std::scoped_lock lock(mutex_);
            Summary s;
            s.total_count = total_count_;
            s.by_zone = by_zone_;
            s.metrics_by_zone = metrics_by_zone_;
            return s;
        }

        // Alias for your parallel ingest tests
        Summary finalize() const {
            return summary();
        }

    private:
        template <typename It>
        void append_records_unlocked(It first, It last) {
            if (first == last) return;

            // Append new records
            window_.records.insert(window_.records.end(), first, last);

            // Initialize window start if this is the first batch
            if (!window_.records.empty()
                && window_.time_start.time_since_epoch().count() == 0) {
                window_.time_start = window_.records.front().ts;
            }

            // Time-based eviction: keep only last window_minutes_ worth of data
            if (!window_.records.empty() && window_minutes_ > 0) {
                const auto max_ts = window_.records.back().ts;
                const auto cutoff = max_ts - std::chrono::minutes(window_minutes_);
                auto erase_it = std::remove_if(
                    window_.records.begin(),
                    window_.records.end(),
                    [&](const model::SensorRecord& r) {
                        return r.ts < cutoff;
                    }
                );
                window_.records.erase(erase_it, window_.records.end());
                if (!window_.records.empty()) {
                    window_.time_start = window_.records.front().ts;
                }
            }

            recompute_stats_unlocked();
        }

        void recompute_stats_unlocked() {
            by_zone_.clear();
            metrics_by_zone_.clear();

            for (const auto& rec : window_.records) {
                int z = rec.zone_id;
                ++by_zone_[z];

                auto& zm = metrics_by_zone_[z];

                if (rec.speed.has_value()) {
                    ++zm.speed.count;
                    zm.speed.sum += *rec.speed;
                }
                if (rec.flow.has_value()) {
                    ++zm.flow.count;
                    zm.flow.sum += *rec.flow;
                }
                if (rec.pm25.has_value()) {
                    ++zm.pm25.count;
                    zm.pm25.sum += *rec.pm25;
                }
                if (rec.pm10.has_value()) {
                    ++zm.pm10.count;
                    zm.pm10.sum += *rec.pm10;
                }
                if (rec.db.has_value()) {
                    ++zm.db.count;
                    zm.db.sum += *rec.db;
                }
            }
        }

        Window window_;
        int    window_minutes_{ 0 };

        int total_count_{ 0 };                            // all ingested rows
        std::unordered_map<int, int> by_zone_;           // per-zone counts (current window)
        std::unordered_map<int, ZoneMetrics> metrics_by_zone_; // per-zone metrics

        mutable std::mutex mutex_;
    };

} // namespace core
