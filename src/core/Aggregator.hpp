#pragma once
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include "Window.hpp"

namespace core {

// Public summary contract used by tests.
struct Summary {
    int total_count{0};
    std::unordered_map<int,int> by_zone; // zone_id -> count
};

class Aggregator {
public:
    explicit Aggregator(int window_minutes)
        : window_minutes_(window_minutes) {}

    // Consume a range of SensorRecord objects. We keep the implementation
    // simple: append incoming records, update zone counts incrementally and
    // evict old records based on the configured time window.
    template <typename Range>
    void consume(const Range& r) {
        // Real (event-time) tumbling windowing.
        // For each incoming record place it into the window bucket determined
        // by its event timestamp (rec.ts). Maintain per-window records and
        // zone counts. Keep only a small retention of recent windows.

        using namespace std::chrono;
        const std::int64_t window_seconds = static_cast<std::int64_t>(window_minutes_) * 60;

        // Insert records into their event-time windows
        for (const auto& rec : r) {
            // compute epoch seconds
            auto secs = duration_cast<seconds>(rec.ts.time_since_epoch()).count();
            std::int64_t win_start = (secs / window_seconds) * window_seconds;
            windows_[win_start].push_back(rec);
            window_zone_counts_[win_start][rec.zone_id]++;

            // update max event time seen
            if (rec.ts > max_event_time_) max_event_time_ = rec.ts;
        }

        if (max_event_time_ == system_clock::time_point{}) {
            // no valid timestamps seen yet -> keep existing state
            return;
        }

        // Determine latest window start based on max_event_time_
        auto max_secs = duration_cast<seconds>(max_event_time_.time_since_epoch()).count();
        std::int64_t latest_win = (max_secs / window_seconds) * window_seconds;

        // Retain only recent windows (keep latest and previous window)
        const std::int64_t retention = window_seconds * 2; // keep two windows
        std::vector<std::int64_t> to_erase;
        for (auto &p : windows_) {
            std::int64_t start = p.first;
            if (start < latest_win - retention) {
                to_erase.push_back(start);
            }
        }
        for (auto s : to_erase) {
            windows_.erase(s);
            window_zone_counts_.erase(s);
        }

        // Set current window view to the latest window
        window_.records.clear();
        zone_counts_.clear();
        auto itw = windows_.find(latest_win);
        if (itw != windows_.end()) {
            window_.records = itw->second;
        }
        auto itc = window_zone_counts_.find(latest_win);
        if (itc != window_zone_counts_.end()) {
            zone_counts_ = itc->second;
        }
    }

    const Window& current_window_view() const { return window_; }

    Summary summary() const {
        Summary s;
        s.total_count = static_cast<int>(window_.records.size());
        s.by_zone = zone_counts_;
        return s;
    }
private:
    Window window_;
    int window_minutes_{0};
    std::unordered_map<int,int> zone_counts_;
    // Real windowing state: map window_start_epoch_seconds -> records and counts
    std::unordered_map<std::int64_t, std::vector<model::SensorRecord>> windows_;
    std::unordered_map<std::int64_t, std::unordered_map<int,int>> window_zone_counts_;
    std::chrono::system_clock::time_point max_event_time_ = std::chrono::system_clock::time_point{};
};

} // namespace core
