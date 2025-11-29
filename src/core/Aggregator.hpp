#pragma once
#include <unordered_map>
#include <mutex>
#include <vector>
#include <algorithm>
#include <chrono>

#include "Window.hpp"
#include "../model/SensorRecord.hpp"
#include "../model/SensorView.hpp"

namespace core {

    // Public summary contract used by tests.
    struct Summary {
        int total_count{ 0 };
        // per-zone count within the current time window
        std::unordered_map<int, int> by_zone;
    };

    // Trait: what counts as a "sensor row" for Aggregator::consume
    template <typename T>
    struct is_sensor_row : std::false_type {};

    template <>
    struct is_sensor_row<model::SensorRecord> : std::true_type {};

    template <>
    struct is_sensor_row<model::SensorView> : std::true_type {};

    template <typename T>
    inline constexpr bool is_sensor_row_v = is_sensor_row<T>::value;

    class Aggregator {
    public:
        explicit Aggregator(int window_minutes)
            : window_minutes_(window_minutes) {
        }

        // Range-based ingest (only SensorRecord / SensorView ranges)
        template <typename Range>
        void consume(const Range& r) {
            using std::begin;
            using std::end;

            auto first = begin(r);
            auto last = end(r);
            if (first == last) return;

            using Value = std::decay_t<decltype(*first)>;
            static_assert(
                is_sensor_row_v<Value>,
                "Aggregator::consume only accepts SensorRecord / SensorView ranges"
                );

            std::scoped_lock lock(mutex_);

            if constexpr (std::is_same_v<Value, model::SensorRecord>) {
                append_records_unlocked(first, last);
            }
            else {
                // SensorView path: promote to owning records
                std::vector<model::SensorRecord> promoted;
                promoted.reserve(std::distance(first, last));
                for (auto it = first; it != last; ++it) {
                    // Any reasonable promotion API is fine; this branch will
                    // only be instantiated if you actually call consume()
                    // with SensorView ranges.
                    promoted.push_back(it->to_record());
                }
                append_records_unlocked(promoted.begin(), promoted.end());
            }

            total_count_ += static_cast<int>(std::distance(first, last));
        }

        // Convenience overload for a single SensorRecord
        void consume(const model::SensorRecord& rec) {
            std::array<model::SensorRecord, 1> one{ rec };
            consume(one);
        }

        // Optional reserve hook used by main.cpp
        void reserve(std::size_t n) {
            std::scoped_lock lock(mutex_);
            window_.records.reserve(n);
        }

        const Window& current_window_view() const {
            return window_;
        }

        Summary summary() const {
            std::scoped_lock lock(mutex_);
            Summary s;
            s.total_count = total_count_;
            s.by_zone = by_zone_;
            return s;
        }

    private:
        template <typename It>
        void append_records_unlocked(It first, It last) {
            if (first == last) return;

            window_.records.insert(window_.records.end(), first, last);

            // Time-based eviction: keep only records in the last window_minutes_
            if (!window_.records.empty() && window_minutes_ > 0) {
                const auto max_ts = window_.records.back().ts;
                const auto cutoff =
                    max_ts - std::chrono::minutes(window_minutes_);

                auto erase_it = std::remove_if(
                    window_.records.begin(),
                    window_.records.end(),
                    [&](const model::SensorRecord& r) {
                        return r.ts < cutoff;
                    });
                window_.records.erase(erase_it, window_.records.end());
            }

            recompute_by_zone_unlocked();
        }

        void recompute_by_zone_unlocked() {
            by_zone_.clear();
            for (const auto& rec : window_.records) {
                ++by_zone_[rec.zone_id];
            }
        }

        Window window_;
        int window_minutes_{ 0 };

        int total_count_{ 0 };
        std::unordered_map<int, int> by_zone_;

        mutable std::mutex mutex_;
    };

} // namespace core
