#pragma once
#include <unordered_map>
#include <mutex>
#include <type_traits>
#include <iterator>
#include <chrono>

#include "Window.hpp"


namespace core {

    struct Summary {
        int total_count{ 0 };
        std::unordered_map<int, int> by_zone; // zone_id -> count
    };

    class Aggregator {
    public:
        explicit Aggregator(int window_minutes)
            : window_minutes_(window_minutes) {
        }

        // NEW: allow callers to pre-reserve space for window records.
        void reserve(std::size_t n) {
            std::lock_guard<std::mutex> lock(mutex_);
            window_.records.reserve(n);
        }

        template <typename Range>
        void consume(const Range& r) {
            using std::begin;
            using std::end;
            using value_type = std::decay_t<decltype(*begin(r))>;

            std::lock_guard<std::mutex> lock(mutex_);

            if constexpr (std::is_same_v<value_type, model::SensorRecord>) {
                for (const auto& rec : r) {
                    window_.records.push_back(rec);
                    summary_.total_count += 1;
                    summary_.by_zone[rec.zone_id] += 1;

                    if (!has_latest_ts_ || rec.ts > latest_ts_) {
                        latest_ts_ = rec.ts;
                        has_latest_ts_ = true;
                    }
                }
                evict_old();
            }
            else {
                const auto c = static_cast<int>(std::distance(begin(r), end(r)));
                summary_.total_count += c;
            }
        }

        // View into current window; intended for single-threaded detector use.
        const Window& current_window_view() const {
            return window_;
        }

        // Snapshot summary (thread-safe copy).
        Summary summary() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return summary_;
        }

    private:
        void evict_old() {
            if (!has_latest_ts_ || window_minutes_ <= 0) return;

            const auto cutoff = latest_ts_ - std::chrono::minutes(window_minutes_);

            // Simple linear eviction from the front.
            auto it = window_.records.begin();
            while (it != window_.records.end() && it->ts < cutoff) {
                // update summary for the evicted record
                summary_.total_count -= 1;
                auto z_it = summary_.by_zone.find(it->zone_id);
                if (z_it != summary_.by_zone.end()) {
                    z_it->second -= 1;
                    if (z_it->second <= 0) {
                        summary_.by_zone.erase(z_it);
                    }
                }

                it = window_.records.erase(it);
            }
        }

        int window_minutes_;
        mutable std::mutex mutex_;
        Window window_;
        Summary summary_;

        std::chrono::system_clock::time_point latest_ts_{};
        bool has_latest_ts_{ false };
    };

} // namespace core
