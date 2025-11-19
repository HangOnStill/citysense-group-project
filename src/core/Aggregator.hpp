#pragma once
#include <unordered_map>
#include <mutex>
#include <type_traits>
#include <iterator>
#include <chrono>
#include "Window.hpp"

namespace core {

    // Public summary contract used by tests.
    struct Summary {
        int total_count{ 0 };
        std::unordered_map<int, int> by_zone; // zone_id -> count
    };

    class Aggregator {
    public:
        explicit Aggregator(int window_minutes)
            : window_minutes_(window_minutes) {
        }

        // Generic consume: works for SensorRecord ranges and also for simple types
        // (tests call consume(std::vector<int>{...})).
        template <typename Range>
        void consume(const Range& r) {
            using std::begin;
            using std::end;

            auto it = begin(r);
            auto it_end = end(r);
            if (it == it_end) return;

            std::scoped_lock lock(mutex_);

            using Value = std::decay_t<decltype(*it)>;

            // Real logic only for SensorRecord ranges
            if constexpr (std::is_same_v<Value, model::SensorRecord>) {
                for (auto cur = it; cur != it_end; ++cur) {
                    window_.records.push_back(*cur);
                }

                // Time-based eviction: keep only records within last window_minutes_
                if (!window_.records.empty() && window_minutes_ > 0) {
                    const auto max_ts = window_.records.back().ts;
                    const auto cutoff =
                        max_ts - std::chrono::minutes(window_minutes_);

                    auto erase_it = std::remove_if(
                        window_.records.begin(),
                        window_.records.end(),
                        [&](const model::SensorRecord& rec) {
                            return rec.ts < cutoff;
                        }
                    );
                    window_.records.erase(erase_it, window_.records.end());
                }

                // Recompute per-zone counts from current window
                recompute_by_zone_unlocked();
            }

            // total_count tracks all ingested elements (any Range value type)
            const int added =
                static_cast<int>(std::distance(it, it_end));
            total_count_ += added;
        }

        const Window& current_window_view() const {
            // NOTE: read-only snapshot; callers should avoid using this
            // concurrently with writes unless they provide external sync.
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
        void recompute_by_zone_unlocked() {
            by_zone_.clear();
            for (const auto& rec : window_.records) {
                ++by_zone_[rec.zone_id];
            }
        }

        Window window_;
        int window_minutes_{ 0 };

        int total_count_{ 0 };                       // all seen elements
        std::unordered_map<int, int> by_zone_;    // counts in current window

        mutable std::mutex mutex_;
    };

} // namespace core
