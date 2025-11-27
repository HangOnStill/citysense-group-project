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

    struct Incident {
        std::string incident_id;
        std::string message;
        model::SensorRecord instance;
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

        // Convenient function that uilizes consume() function and then filters the current
        // window's records by zone. 
        template <typename Range>
        void consume_by_zone(const Range& r, int zone_id_) {
            consume(r);
            int no_removed = 0;

            std::scoped_lock lock(mutex_);
            auto erase_it = std::remove_if(
                window_.records.begin(),
                window_.records.end(),
                [&](const model::SensorRecord& rec) {
                    if (rec.zone_id != zone_id_) {
                        ++no_removed;
                        return true;
                    }
                    return false;
                }
            );
            window_.records.erase(erase_it, window_.records.end());
            total_count_ -= no_removed;
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
        
        const std::vector<Incident>& get_incidents() const {
            return incidents;
        }
        void record_incident(const Incident& incident) {
            std::scoped_lock lock(mutex_);
            incidents.push_back(incident);
        }
        void clear_incidents() {
            incidents.clear();
        }

        // Computes the mean speed, flow, pm25, pm10, and db by each zone in a SensorRecord range.
        // A hashmap is returned, where each zone has a corresponding hashmap containing its averaged
        // metrics.
        using MetricsMap = std::unordered_map<std::string, double>;
        // CountMap to help compute denominators, in order to calculate the mean.
        using CountMap = std::unordered_map<std::string, int>;

        template <typename Range>
        std::unordered_map<int, MetricsMap> compute_means(const Range& r) {

            auto it = std::begin(r);
            auto it_end = std::end(r);
            if (it == it_end) return {};

            std::unordered_map<int, MetricsMap> result;
            std::unordered_map<int, CountMap> counter;

            using Value = std::decay_t<decltype(*it)>;
            if constexpr (std::is_same_v<Value, model::SensorRecord>) {

                for (auto cur=it; cur != it_end; cur++) {
                    model::SensorRecord rec = *cur;
                    if (rec.speed.has_value()) {
                        result[rec.zone_id]["speed"] += rec.speed.value(); counter[rec.zone_id]["speed"]++;
                    }
                    if (rec.flow.has_value()) {
                        result[rec.zone_id]["flow"] += rec.flow.value(); counter[rec.zone_id]["flow"]++;
                    }
                    if (rec.pm25.has_value()) {
                        result[rec.zone_id]["pm25"] += rec.pm25.value(); counter[rec.zone_id]["pm25"]++;
                    }
                    if (rec.pm10.has_value()) {
                        result[rec.zone_id]["pm10"] += rec.pm10.value(); counter[rec.zone_id]["pm10"]++;
                    }
                    if (rec.db.has_value()) {
                        result[rec.zone_id]["db"] += rec.db.value(); counter[rec.zone_id]["db"]++;
                    }
                }
            }
            // Calculate means in each zone
            for (const auto& [zone, metrics] : counter) {
                for (const auto& [metric, denominator] : metrics) {

                    if (denominator != 0) {
                        double& numerator = result[zone][metric];
                        numerator = numerator / static_cast<double>(denominator);
                    }
                }
            }
            return result;
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

        std::vector<Incident> incidents;
    };

} // namespace core
