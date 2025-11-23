#pragma once
#include <vector>
#include <map>
#include <string>
#include <optional>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <deque>
#include "../model/SensorRecord.hpp"

using namespace std;

namespace core {

// Statistics for a bucket of readings
struct BucketStats {
    chrono::system_clock::time_point bucket_start;
    int zone_id;
    size_t count = 0;
    
    // Per-metric statistics (only populated if data exists)
    optional<double> speed_mean;
    optional<double> speed_median;
    optional<double> speed_p90;
    optional<double> speed_p99;
    
    optional<double> flow_mean;
    optional<double> flow_median;
    optional<double> flow_p90;
    optional<double> flow_p99;
    
    optional<double> pm25_mean;
    optional<double> pm25_median;
    optional<double> pm25_p90;
    optional<double> pm25_p99;
    
    optional<double> pm10_mean;
    optional<double> pm10_median;
    optional<double> pm10_p90;
    optional<double> pm10_p99;
    
    optional<double> db_mean;
    optional<double> db_median;
    optional<double> db_p90;
    optional<double> db_p99;
};

// Rolling average tracker per sensor
struct RollingAverage {
    vector<double> values;
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
        return accumulate(values.begin(), values.end(), 0.0) / values.size();
    }
};

class Processor {
public:
    struct Config {
        vector<int> zone_filter;  // Empty = all zones
        optional<chrono::system_clock::time_point> start_time;
        optional<chrono::system_clock::time_point> end_time;
        int bucket_minutes = 5;
        int rolling_window_size = 5;
        bool apply_rolling_avg = true;
    };
    
    explicit Processor(const Config& config) : config_(config) {}
    
    // Process a batch of records
    void process_batch(const vector<model::SensorRecord>& records);
    
    // Get computed statistics
    vector<BucketStats> get_bucket_stats() const;
    
    // Get diagnostics
    struct Diagnostics {
        size_t total_records = 0;
        size_t filtered_out = 0;
        size_t processed = 0;
        vector<string> warnings;
    };
    
    const Diagnostics& get_diagnostics() const { return diag_; }
    
private:
    Config config_;
    Diagnostics diag_;
    
    // Rolling averages per sensor per metric
    map<string, RollingAverage> speed_rolling_;
    map<string, RollingAverage> flow_rolling_;
    map<string, RollingAverage> pm25_rolling_;
    map<string, RollingAverage> pm10_rolling_;
    map<string, RollingAverage> db_rolling_;
    
    // Bucketed data: key = (bucket_start_timestamp, zone_id)
    struct BucketKey {
        chrono::system_clock::time_point bucket_start;
        int zone_id;
        
        bool operator<(const BucketKey& other) const {
            if (bucket_start != other.bucket_start)
                return bucket_start < other.bucket_start;
            return zone_id < other.zone_id;
        }
    };
    
    struct BucketData {
        vector<double> speed_values;
        vector<double> flow_values;
        vector<double> pm25_values;
        vector<double> pm10_values;
        vector<double> db_values;
    };
    
    map<BucketKey, BucketData> buckets_;
    
    // Helper methods
    bool passes_filters(const model::SensorRecord& record) const;
    chrono::system_clock::time_point get_bucket_start(
        const chrono::system_clock::time_point& timestamp) const;
    model::SensorRecord apply_transformations(const model::SensorRecord& record);
    void compute_stats(const vector<double>& values,
                      optional<double>& mean,
                      optional<double>& median,
                      optional<double>& p90,
                      optional<double>& p99) const;
};

} // namespace core
