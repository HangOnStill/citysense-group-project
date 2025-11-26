#pragma once
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <vector>
#include <type_traits>
#include <concepts>
#include <ranges>
#include <string>
#include "Window.hpp"
#include "../model/SensorView.hpp"

namespace core {

    // Public summary contract used by tests.
    struct Summary {
        int total_count{ 0 };
        std::unordered_map<int, int> by_zone; // zone_id -> count
    };

    // Concept for the two allowed sensor-row types.
    template <typename T>
    concept SensorRow =
        std::same_as<std::remove_cvref_t<T>, model::SensorRecord> ||
        std::same_as<std::remove_cvref_t<T>, model::SensorView>;

    // A range of sensor rows (value or view).
    template <typename R>
    concept SensorRowRange =
        std::ranges::input_range<R> &&
        SensorRow<std::ranges::range_value_t<R>>;

    // Thread-safe time-windowed aggregator over sensor readings.
    // Maintains a sliding window of the last `window_minutes` minutes of data
    // based on *event time* (the timestamps inside the records, not wall-clock).
    class Aggregator {
    public:
        explicit Aggregator(int window_minutes)
            : window_minutes_{ window_minutes }
        {
        }

        // Ingest a batch of sensor rows. Accepts both SensorRecord and SensorView
        // ranges, but nothing else (see SensorRowRange concept above).
        template <SensorRowRange Range>
        void consume(const Range& rows) {
            using namespace std::chrono;
            std::lock_guard<std::mutex> lock(mutex_);

            if (window_minutes_ <= 0) {
                // Degenerate: treat as "no eviction" infinite window.
                // Still updates internal counters deterministically.
                for (const auto& row : rows) {
                    add_record_unlocked(row);
                }
                rebuild_window_unlocked();
                return;
            }

            for (const auto& row : rows) {
                add_record_unlocked(row);
            }

            // After ingesting this batch, evict buckets that fall completely
            // outside the sliding window [max_event_time_ - window_minutes_, max_event_time_].
            if (max_event_time_ != system_clock::time_point{}) {
                auto cutoff_time = max_event_time_ - minutes(window_minutes_);
                auto cutoff_epoch = duration_cast<seconds>(cutoff_time.time_since_epoch()).count();

                for (auto it = windows_.begin(); it != windows_.end(); ) {
                    if (it->first < cutoff_epoch) {
                        window_zone_counts_.erase(it->first);
                        it = windows_.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }

            rebuild_window_unlocked();
        }

        // View over the current time window.
        // Note: this is not internally synchronised for long-lived use; callers
        // should avoid concurrent mutation while holding the reference.
        const Window& current_window_view() const {
            return window_;
        }

        // Thread-safe snapshot summary of the current window.
        Summary summary() const {
            std::lock_guard<std::mutex> lock(mutex_);
            Summary s;
            s.total_count = static_cast<int>(window_.records.size());
            s.by_zone = zone_counts_;
            return s;
        }

    private:
        using sys_clock_t = std::chrono::system_clock;

        template <SensorRow Row>
        void add_record_unlocked(const Row& row) {
            using namespace std::chrono;

            // Update max event-time seen so far.
            if (row.ts > max_event_time_) {
                max_event_time_ = row.ts;
            }

            // Compute a per-minute bucket key in epoch seconds.
            const auto epoch_seconds =
                duration_cast<seconds>(row.ts.time_since_epoch()).count();
            const std::int64_t bucket_start =
                (epoch_seconds / 60) * 60; // start-of-minute boundary

            // Materialise into a value-type SensorRecord so the aggregator can
            // safely outlive any backing buffers (when Row is SensorView).
            model::SensorRecord stored{};
            stored.ts = row.ts;
            stored.sensor_id = std::string(row.sensor_id);
            stored.zone_id = row.zone_id;
            stored.speed = row.speed;
            stored.flow = row.flow;
            stored.pm25 = row.pm25;
            stored.pm10 = row.pm10;
            stored.db = row.db;

            windows_[bucket_start].push_back(std::move(stored));
            window_zone_counts_[bucket_start][row.zone_id] += 1;
        }

        // Rebuilds the flattened Window + zone_counts_ from the bucketed state.
        void rebuild_window_unlocked() {
            window_.records.clear();
            zone_counts_.clear();

            for (auto& [bucket_start, recs] : windows_) {
                (void)bucket_start; // unused but kept for clarity
                for (const auto& rec : recs) {
                    window_.records.push_back(rec);
                    zone_counts_[rec.zone_id] += 1;
                }
            }
        }

    private:
        mutable std::mutex mutex_;

        Window window_;
        int window_minutes_{ 0 };
        std::unordered_map<int, int> zone_counts_;

        // Real windowing state: map window_start_epoch_seconds -> records and counts
        std::unordered_map<std::int64_t, std::vector<model::SensorRecord>> windows_;
        std::unordered_map<std::int64_t, std::unordered_map<int, int>> window_zone_counts_;
        sys_clock_t::time_point max_event_time_ = sys_clock_t::time_point{};
    };

} // namespace core
