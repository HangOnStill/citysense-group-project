#pragma once
#include <vector>
#include <map>
#include <string>
#include <optional>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include "../model/SensorRecord.hpp"

namespace core {

// Statistics for a bucket of readings
struct BucketStats {
    std::chrono::system_clock::time_point bucket_start;
    int zone_id;
    size_t count = 0;
    
    // Per-metric statistics (only populated if data exists)
    std::optional<double> speed_mean;
    std::optional<double> speed_median;
    std::optional<double> speed_p90;
    std::optional<double> speed_p99;
    
    std::optional<double> flow_mean;
    std::optional<double> flow_median;
    std::optional<double> flow_p90;
    std::optional<double> flow_p99;
    
    std::optional<double> pm25_mean;
    std::optional<double> pm25_median;
    std::optional<double> pm25_p90;
    std::optional<double> pm25_p99;
    
    std::optional<double> pm10_mean;
    std::optional<double> pm10_median;
    std::optional<double> pm10_p90;
    std::optional<double> pm10_p99;
    
    std::optional<double> db_mean;
    std::optional<double> db_median;
    std::optional<double> db_p90;
    std::optional<double> db_p99;
};

// Rolling average tracker per sensor
struct RollingAverage {
    std::vector<double> values;
    size_t max_size;
    
    explicit RollingAverage(size_t window_size = 5) : max_size(window_size) {}
    
    void add(double value) {
        values.push_back(value);
        if (values.size() > max_size) {
            values.erase(values.begin());
        }
    }
    
    double get_average() const {
        if (values.empty()) return 0.0;
        return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }
};

class Processor {
public:
    struct Config {
        std::vector<int> zone_filter;  // Empty = all zones
        std::optional<std::chrono::system_clock::time_point> start_time;
        std::optional<std::chrono::system_clock::time_point> end_time;
        int bucket_minutes = 5;
        int rolling_window_size = 5;  // Number of readings for rolling average
        bool apply_rolling_avg = true;
    };
    
    explicit Processor(const Config& config) : config_(config) {}
    
    // Process a batch of records
    void process_batch(const std::vector<model::SensorRecord>& records);
    
    // Get computed statistics
    std::vector<BucketStats> get_bucket_stats() const;
    
    // Get diagnostics
    struct Diagnostics {
        size_t total_records = 0;
        size_t filtered_by_time = 0;
        size_t filtered_by_zone = 0;
        size_t processed = 0;
        std::vector<std::string> warnings;
    };
    
    const Diagnostics& get_diagnostics() const { return diag_; }
    
private:
    Config config_;
    Diagnostics diag_;
    
    // Rolling averages per sensor per metric
    std::map<std::string, RollingAverage> speed_rolling_;
    std::map<std::string, RollingAverage> flow_rolling_;
    std::map<std::string, RollingAverage> pm25_rolling_;
    std::map<std::string, RollingAverage> pm10_rolling_;
    std::map<std::string, RollingAverage> db_rolling_;
    
    // Bucketed data: key = (bucket_start_timestamp, zone_id)
    struct BucketKey {
        std::chrono::system_clock::time_point bucket_start;
        int zone_id;
        
        bool operator<(const BucketKey& other) const {
            if (bucket_start != other.bucket_start)
                return bucket_start < other.bucket_start;
            return zone_id < other.zone_id;
        }
    };
    
    struct BucketData {
        std::vector<double> speed_values;
        std::vector<double> flow_values;
        std::vector<double> pm25_values;
        std::vector<double> pm10_values;
        std::vector<double> db_values;
    };
    
    std::map<BucketKey, BucketData> buckets_;
    
    // Helper methods
    bool passes_filters(const model::SensorRecord& record) const;
    std::chrono::system_clock::time_point get_bucket_start(
        const std::chrono::system_clock::time_point& timestamp) const;
    model::SensorRecord apply_transformations(const model::SensorRecord& record);
    void compute_stats(const std::vector<double>& values,
                      std::optional<double>& mean,
                      std::optional<double>& median,
                      std::optional<double>& p90,
                      std::optional<double>& p99) const;
};

} // namespace core
