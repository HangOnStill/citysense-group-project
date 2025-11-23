#include "Processor.hpp"
#include <algorithm>

using namespace std;

namespace core {

// TODO: Implement process_batch()
// This is the main function that processes all records
// For each record:
//   1. Increment diag_.total_records counter
//   2. Check if it passes filters (call passes_filters())
//   3. If filtered out, continue to next record
//   4. Apply transformations (call apply_transformations())
//   5. Get the bucket start time (call get_bucket_start())
//   6. Create BucketKey with bucket_start and zone_id
//   7. Get reference to bucket in buckets_ map
//   8. Add each metric value (if present) to appropriate bucket vector
//   9. Increment diag_.processed counter
void Processor::process_batch(const vector<model::SensorRecord>& records) {
    for(const auto& record : records){
        diag_.total_records++;

        if(!passes_filters(record)){
            continue;
        }

        model::SensorRecord transformed = apply_transformations(record);

        auto bucket_start = get_bucket_start(transformed.ts);

        BucketKey key{bucket_start, transformed.zone_id};

        auto& bucket = buckets_[key];
        //adding each metric to bucket vectors if present
        if(transformed.speed.has_value()){
            bucket.speed_values.push_back(transformed.speed.value());
        }
        if(transformed.flow.has_value()){
            bucket.flow_values.push_back(transformed.flow.value());
        }
        if(transformed.pm25.has_value()){
            bucket.pm25_values.push_back(transformed.pm25.value());
        }
        if(transformed.pm10.has_value()){
            bucket.pm10_values.push_back(transformed.pm10.value());
        }
        if(transformed.db.has_value()){
            bucket.db_values.push_back(transformed.db.value());
        }
        diag_.processed++;
    }
}

// TODO: Implement get_bucket_stats()
// This converts the raw bucket data into computed statistics
// Steps:
//   1. Create empty vector<BucketStats> results
//   2. Loop through all buckets in buckets_ map
//   3. For each bucket:
//      - Create BucketStats object
//      - Set bucket_start and zone_id from key
//      - Calculate count (sum of all metric vector sizes)
//      - Call compute_stats() for each metric to fill in mean/median/p90/p99
//      - Add to results vector
//   4. Return results
vector<BucketStats> Processor::get_bucket_stats() const {
    vector<BucketStats> results;

    // loop through all the buckets
    for(const auto& [key, bucket] : buckets_){
        BucketStats stats;

        stats.bucket_start = key.bucket_start;
        stats.zone_id = key.zone_id;
        //calculates total sum of all metrics
        stats.count = bucket.speed_values.size() + 
                      bucket.flow_values.size() + 
                      bucket.pm25_values.size() + 
                      bucket.pm10_values.size() + 
                      bucket.db_values.size();

                          if(!bucket.speed_values.empty()){
        compute_stats(bucket.speed_values,
                      stats.speed_mean,
                      stats.speed_median,
                      stats.speed_p90,
                      stats.speed_p99);
    }
    if(!bucket.flow_values.empty()){
        compute_stats(bucket.flow_values,
                      stats.flow_mean,
                      stats.flow_median,
                      stats.flow_p90,
                      stats.flow_p99);  
                    }
    if(!bucket.pm25_values.empty()){
        compute_stats(bucket.pm25_values,
        stats.pm25_mean, stats.pm25_median,
        stats.pm25_p90, stats.pm25_p99);
    }

    // PM10 stats
    if(!bucket.pm10_values.empty()){
        compute_stats(bucket.pm10_values,
        stats.pm10_mean, stats.pm10_median,
    stats.pm10_p90, stats.pm10_p99);
    }
    // dB stats
    if(!bucket.db_values.empty()){
        compute_stats(bucket.db_values,
        stats.db_mean, stats.db_median,
        stats.db_p90, stats.db_p99);
    }

        results.push_back(stats);
    }

    return results;

}

// TODO: Implement passes_filters()
// Check if a record passes time and zone filters
// Steps:
//   1. Check time filters:
//      - If start_time is set and record.timestamp < start_time, reject
//      - If end_time is set and record.timestamp > end_time, reject
//      - Increment diag_.filtered_by_time when rejecting
//   2. Check zone filter:
//      - If zone_filter is not empty, check if record.zone_id is in the list
//      - Use find() to search vector
//      - Increment diag_.filtered_by_zone when rejecting
//   3. Return true if passes all filters
// Note: Need const_cast to modify diag_ counters in const function
bool Processor::passes_filters(const model::SensorRecord& record) const {
    // Filter 1: Check start time
    if(config_.start_time.has_value() && record.ts < config_.start_time.value()){
        const_cast<Diagnostics&>(diag_).filtered_out++;
        return false;
    }
    
    // Filter 2: Check end time
    if(config_.end_time.has_value() && record.ts > config_.end_time.value()){
        const_cast<Diagnostics&>(diag_).filtered_out++;
        return false;
    }
    
    // Filter 3: Check zone
    if(!config_.zone_filter.empty()){
        bool found = find(config_.zone_filter.begin(), config_.zone_filter.end(), record.zone_id) != config_.zone_filter.end();
        if(!found){
            const_cast<Diagnostics&>(diag_).filtered_out++;
            return false;
        }
    }
    
    // Passed all filters!
    return true;
}

// TODO: Implement get_bucket_start()
// Round timestamp down to nearest bucket boundary
// Steps:
//   1. Convert timestamp to minutes since epoch
//      - Use chrono::duration_cast<chrono::minutes>()
//      - Get .count() to get the number
//   2. Calculate bucket boundary:
//      - Divide by bucket_minutes, then multiply back
//      - This rounds down to nearest multiple
//   3. Convert back to time_point using chrono::minutes
//   4. Return the time_point
chrono::system_clock::time_point Processor::get_bucket_start(
    const chrono::system_clock::time_point& timestamp) const {
    auto minutes_since_epoch = chrono::duration_cast<chrono::minutes>(
        timestamp.time_since_epoch()
    ).count();

    long long bucket_number = minutes_since_epoch / config_.bucket_minutes;
    long long bucket_start_minutes = bucket_number * config_.bucket_minutes;

    return chrono::system_clock::time_point(chrono::minutes(bucket_start_minutes));
}

// TODO: Implement apply_transformations()
// Apply rolling averages to sensor readings
// Steps:
//   1. If apply_rolling_avg is false, return record unchanged
//   2. Create a copy of the record called transformed
//   3. Get the sensor_id from the record
//   4. For each metric (speed, flow, pm25, pm10, db):
//      - If the metric has a value:
//        a. Add it to the appropriate rolling average map (e.g., speed_rolling_[sensor_id].add())
//        b. Replace the value in transformed with get_average()
//   5. Return the transformed record
model::SensorRecord Processor::apply_transformations(const model::SensorRecord& record) {
    // checks if rolling averages are enabled
    if(!config_.apply_rolling_avg){
        return record;
    }
    model::SensorRecord transformed = record; // creates a copy of the record to modify

    string sensor_id = record.sensor_id;

    // processes each metric one by one

    // speed
    if(record.speed.has_value()){
        speed_rolling_[sensor_id].add(record.speed.value());
        transformed.speed = speed_rolling_[sensor_id].get_average();
    }
    //flow
    if(record.flow.has_value()){
        flow_rolling_[sensor_id].add(record.flow.value());
        transformed.flow = flow_rolling_[sensor_id].get_average();
    }
    // PM2.5
    if(record.pm25.has_value()){
        pm25_rolling_[sensor_id].add(record.pm25.value());
        transformed.pm25 = pm25_rolling_[sensor_id].get_average();
    }
    // PM10
    if(record.pm10.has_value()){
        pm10_rolling_[sensor_id].add(record.pm10.value());
        transformed.pm10 = pm10_rolling_[sensor_id].get_average();
    }

    //Decibels (dB)
    if(record.db.has_value()){
        db_rolling_[sensor_id].add(record.db.value());
        transformed.db = db_rolling_[sensor_id].get_average();
    }

    return transformed;
    
}

// TODO: Implement compute_stats()
// Calculate mean, median, p90, p99 for a vector of values
// Steps:
//   1. If values is empty, return (leave all optionals unset)
//   2. Calculate mean:
//      - Use accumulate() to sum all values
//      - Divide by values.size()
//      - Assign to mean parameter
//   3. For percentiles, make a sorted copy of values
//   4. Calculate median:
//      - Find middle index (size / 2)
//      - If even size, average the two middle values
//      - If odd size, take the middle value
//   5. Calculate p90:
//      - Index = ceil(0.90 * size) - 1
//      - Get value at that index
//   6. Calculate p99:
//      - Index = ceil(0.99 * size) - 1
//      - Get value at that index
void Processor::compute_stats(const vector<double>& values,
                              optional<double>& mean,
                              optional<double>& median,
                              optional<double>& p90,
                              optional<double>& p99) const {
    if(values.empty()){
        return;
    }
    double sum = accumulate(values.begin(), values.end(), 0.0);

    mean = sum / values.size();

    vector<double> sorted = values;
    sort(sorted.begin(), sorted.end());

    size_t n = sorted.size();
    size_t mid = n/2;

    if(n % 2 == 0){
        median = (sorted[mid - 1] + sorted[mid]) / 2.0;
    }else{
        median = sorted[mid];
    }
    // 90th percentile values are 90% or below
    size_t idx_p90 = static_cast<size_t>((ceil(0.90 * n)) - 1);
    // make sure index is in bound
    if(idx_p90 >= n) idx_p90 = n - 1;
    p90 = sorted[idx_p90];

    size_t idx_p99 = static_cast<size_t>(ceil(0.99 * n)) -1;

    if (idx_p99 >= n) idx_p99 = n-1;
    p99 = sorted[idx_p99];
}

} // namespace core
