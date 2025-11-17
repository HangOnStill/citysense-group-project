#include "Processor.hpp"
#include <algorithm>

namespace core {

void Processor::process_batch(const std::vector<model::SensorRecord>& records) {
    for (const auto& record : records) {
        diag_.total_records++;
        
        // Apply filters
        if (!passes_filters(record)) {
            continue;
        }
        
        // Apply transformations (rolling averages)
        auto transformed = apply_transformations(record);
        
        // Determine bucket
        auto bucket_start = get_bucket_start(transformed.ts);
        BucketKey key{bucket_start, transformed.zone_id};
        
        // Add to bucket
        auto& bucket = buckets_[key];
        
        if (transformed.speed.has_value()) {
            bucket.speed_values.push_back(transformed.speed.value());
        }
        if (transformed.flow.has_value()) {
            bucket.flow_values.push_back(transformed.flow.value());
        }
        if (transformed.pm25.has_value()) {
            bucket.pm25_values.push_back(transformed.pm25.value());
        }
        if (transformed.pm10.has_value()) {
            bucket.pm10_values.push_back(transformed.pm10.value());
        }
        if (transformed.db.has_value()) {
            bucket.db_values.push_back(transformed.db.value());
        }
        
        diag_.processed++;
    }
}

std::vector<BucketStats> Processor::get_bucket_stats() const {
    std::vector<BucketStats> results;
    
    for (const auto& [key, data] : buckets_) {
        BucketStats stats;
        stats.bucket_start = key.bucket_start;
        stats.zone_id = key.zone_id;
        stats.count = data.speed_values.size() + data.flow_values.size() +
                      data.pm25_values.size() + data.pm10_values.size() +
                      data.db_values.size();
        
        // Compute statistics for each metric
        compute_stats(data.speed_values, stats.speed_mean, stats.speed_median,
                     stats.speed_p90, stats.speed_p99);
        compute_stats(data.flow_values, stats.flow_mean, stats.flow_median,
                     stats.flow_p90, stats.flow_p99);
        compute_stats(data.pm25_values, stats.pm25_mean, stats.pm25_median,
                     stats.pm25_p90, stats.pm25_p99);
        compute_stats(data.pm10_values, stats.pm10_mean, stats.pm10_median,
                     stats.pm10_p90, stats.pm10_p99);
        compute_stats(data.db_values, stats.db_mean, stats.db_median,
                     stats.db_p90, stats.db_p99);
        
        results.push_back(stats);
    }
    
    return results;
}

bool Processor::passes_filters(const model::SensorRecord& record) const {
    // Time filter
    if (config_.start_time.has_value() && record.ts < config_.start_time.value()) {
        const_cast<Processor*>(this)->diag_.filtered_by_time++;
        return false;
    }
    if (config_.end_time.has_value() && record.ts > config_.end_time.value()) {
        const_cast<Processor*>(this)->diag_.filtered_by_time++;
        return false;
    }
    
    // Zone filter
    if (!config_.zone_filter.empty()) {
        bool zone_match = std::find(config_.zone_filter.begin(),
                                   config_.zone_filter.end(),
                                   record.zone_id) != config_.zone_filter.end();
        if (!zone_match) {
            const_cast<Processor*>(this)->diag_.filtered_by_zone++;
            return false;
        }
    }
    
    return true;
}

std::chrono::system_clock::time_point Processor::get_bucket_start(
    const std::chrono::system_clock::time_point& timestamp) const {
    
    auto minutes_since_epoch = std::chrono::duration_cast<std::chrono::minutes>(
        timestamp.time_since_epoch()).count();
    
    // Round down to nearest bucket boundary
    auto bucket_minutes = (minutes_since_epoch / config_.bucket_minutes) * config_.bucket_minutes;
    
    return std::chrono::system_clock::time_point(std::chrono::minutes(bucket_minutes));
}

model::SensorRecord Processor::apply_transformations(const model::SensorRecord& record) {
    model::SensorRecord transformed = record;
    
    if (!config_.apply_rolling_avg) {
        return transformed;
    }
    
    // Apply rolling averages per sensor
    const std::string& sensor_id = record.sensor_id;
    
    if (record.speed.has_value()) {
        auto& rolling = speed_rolling_[sensor_id];
        if (rolling.max_size == 0) rolling.max_size = config_.rolling_window_size;
        rolling.add(record.speed.value());
        transformed.speed = rolling.get_average();
    }
    
    if (record.flow.has_value()) {
        auto& rolling = flow_rolling_[sensor_id];
        if (rolling.max_size == 0) rolling.max_size = config_.rolling_window_size;
        rolling.add(record.flow.value());
        transformed.flow = rolling.get_average();
    }
    
    if (record.pm25.has_value()) {
        auto& rolling = pm25_rolling_[sensor_id];
        if (rolling.max_size == 0) rolling.max_size = config_.rolling_window_size;
        rolling.add(record.pm25.value());
        transformed.pm25 = rolling.get_average();
    }
    
    if (record.pm10.has_value()) {
        auto& rolling = pm10_rolling_[sensor_id];
        if (rolling.max_size == 0) rolling.max_size = config_.rolling_window_size;
        rolling.add(record.pm10.value());
        transformed.pm10 = rolling.get_average();
    }
    
    if (record.db.has_value()) {
        auto& rolling = db_rolling_[sensor_id];
        if (rolling.max_size == 0) rolling.max_size = config_.rolling_window_size;
        rolling.add(record.db.value());
        transformed.db = rolling.get_average();
    }
    
    return transformed;
}

void Processor::compute_stats(const std::vector<double>& values,
                              std::optional<double>& mean,
                              std::optional<double>& median,
                              std::optional<double>& p90,
                              std::optional<double>& p99) const {
    if (values.empty()) {
        return;
    }
    
    // Mean
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    mean = sum / values.size();
    
    // For percentiles, we need a sorted copy
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    
    // Median (p50)
    size_t mid = sorted.size() / 2;
    if (sorted.size() % 2 == 0) {
        median = (sorted[mid - 1] + sorted[mid]) / 2.0;
    } else {
        median = sorted[mid];
    }
    
    // P90 (90th percentile)
    size_t idx_90 = static_cast<size_t>(std::ceil(0.90 * sorted.size())) - 1;
    if (idx_90 < sorted.size()) {
        p90 = sorted[idx_90];
    }
    
    // P99 (99th percentile)
    size_t idx_99 = static_cast<size_t>(std::ceil(0.99 * sorted.size())) - 1;
    if (idx_99 < sorted.size()) {
        p99 = sorted[idx_99];
    }
}

} // namespace core
