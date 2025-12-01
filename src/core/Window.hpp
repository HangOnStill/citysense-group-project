// src/core/Window.hpp
#pragma once

#include <vector>
#include <chrono>
#include "model/SensorRecord.hpp"

namespace core {

    class Window {   // must be 'class' to match Detector.hpp forward declaration
    public:
        using clock = std::chrono::system_clock;
        using time_point = clock::time_point;

        // These two fields are what Aggregator/tests already expect:
        time_point time_start{};                     // window start (set by Aggregator)
        std::vector<model::SensorRecord> records;    // all records in this window

        Window() = default;

        // --- Convenience helpers (NEW) ---

        bool empty() const { return records.empty(); }
        std::size_t size() const { return records.size(); }

        void add_record(const model::SensorRecord& rec) {
            records.push_back(rec);
        }

        // Allow range-for: for (const auto& r : window) { ... }
        auto begin() const { return records.begin(); }
        auto end()   const { return records.end(); }

        // Derived timestamps for detectors
        time_point start_ts() const {
            if (!records.empty()) {
                return records.front().ts;
            }
            return time_start;  // fallback
        }

        time_point end_ts() const {
            if (!records.empty()) {
                return records.back().ts;
            }
            return time_start;  // fallback
        }

        void clear() { records.clear(); }
    };

} // namespace core
