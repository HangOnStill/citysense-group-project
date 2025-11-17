#include <iostream>
#include <cassert>
#include <cmath>
#include "src/io/ReaderCSV.hpp"
#include "src/io/ReaderJSON.hpp"
#include "src/core/Processor.hpp"

// Test utilities
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        std::cout << "Running: " << #name << "... "; \
        try { \
            test_##name(); \
            std::cout << "PASSED\n"; \
            tests_passed++; \
        } catch (const std::exception& e) { \
            std::cout << "FAILED: " << e.what() << "\n"; \
            tests_failed++; \
        } \
    } \
    void test_##name()

#define ASSERT_TRUE(expr) \
    if (!(expr)) throw std::runtime_error(#expr " is false")

#define ASSERT_FALSE(expr) \
    if (expr) throw std::runtime_error(#expr " is true")

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) throw std::runtime_error(std::string(#a " != " #b))

#define ASSERT_NEAR(a, b, epsilon) \
    if (std::abs((a) - (b)) > (epsilon)) throw std::runtime_error(#a " not near " #b)

// ============= ReaderCSV Tests =============

TEST(csv_reads_air_data) {
    io::ReaderCSV reader({"data/air.csv"});
    auto batch = reader.next_batch(10);
    
    ASSERT_TRUE(batch.size() >= 1);
    ASSERT_TRUE(batch[0].pm25.has_value());
    ASSERT_TRUE(batch[0].pm10.has_value());
    ASSERT_EQ(batch[0].zone_id, 1);
}

TEST(csv_reads_noise_data) {
    io::ReaderCSV reader({"data/noise.csv"});
    auto batch = reader.next_batch(10);
    
    ASSERT_TRUE(batch.size() >= 1);
    ASSERT_TRUE(batch[0].db.has_value());
    ASSERT_FALSE(batch[0].pm25.has_value());
}

TEST(csv_reads_traffic_data) {
    io::ReaderCSV reader({"data/traffic.csv"});
    auto batch = reader.next_batch(10);
    
    ASSERT_TRUE(batch.size() >= 1);
    ASSERT_TRUE(batch[0].speed.has_value());
    ASSERT_TRUE(batch[0].flow.has_value());
    ASSERT_FALSE(batch[0].db.has_value());
}

TEST(csv_handles_multiple_files) {
    io::ReaderCSV reader({"data/air.csv", "data/noise.csv"});
    auto batch = reader.next_batch(100);
    
    ASSERT_TRUE(batch.size() > 7); // Should have data from both files
}

TEST(csv_tracks_parse_stats) {
    io::ReaderCSV reader({"data/air.csv"});
    auto batch = reader.next_batch(100);
    auto stats = reader.get_stats();
    
    ASSERT_TRUE(stats.total_rows > 0);
    ASSERT_TRUE(stats.parsed_rows > 0);
    ASSERT_EQ(stats.parsed_rows, batch.size());
}

TEST(csv_parses_timestamps) {
    io::ReaderCSV reader({"data/air.csv"});
    auto batch = reader.next_batch(2);
    
    ASSERT_TRUE(batch.size() >= 2);
    // Second timestamp should be after first
    ASSERT_TRUE(batch[1].ts > batch[0].ts);
}

TEST(csv_case_insensitive_headers) {
    // Our CSV files use lowercase, but parser should handle any case
    io::ReaderCSV reader({"data/traffic.csv"});
    auto batch = reader.next_batch(1);
    
    ASSERT_TRUE(batch.size() >= 1);
    ASSERT_TRUE(!batch[0].sensor_id.empty());
}

// ============= ReaderJSON Tests =============

TEST(json_reads_config) {
    io::ReaderJSON reader("data/config.json");
    auto config = reader.parse_config();
    
    ASSERT_TRUE(!config.inputs.empty());
    ASSERT_EQ(config.window_minutes, 5);
}

TEST(json_parses_inputs_array) {
    io::ReaderJSON reader("data/config.json");
    auto config = reader.parse_config();
    
    ASSERT_TRUE(config.inputs.size() >= 1);
    ASSERT_EQ(config.inputs[0], "data/air.csv");
}

TEST(json_uses_defaults) {
    io::ReaderJSON reader("data/config.json");
    auto config = reader.parse_config();
    
    // Should have default values for fields not in config.json
    ASSERT_EQ(config.out_path, "citysense_report.json");
}

// ============= Processor Tests =============

TEST(processor_filters_by_zone) {
    core::Processor::Config config;
    config.zone_filter = {1}; // Only zone 1
    config.bucket_minutes = 5;
    config.apply_rolling_avg = false;
    
    core::Processor processor(config);
    
    io::ReaderCSV reader({"data/air.csv"});
    auto batch = reader.next_batch(100);
    processor.process_batch(batch);
    
    auto diag = processor.get_diagnostics();
    ASSERT_TRUE(diag.filtered_by_zone > 0); // Some records should be filtered
    ASSERT_TRUE(diag.processed > 0); // Some should pass
}

TEST(processor_creates_buckets) {
    core::Processor::Config config;
    config.bucket_minutes = 5;
    config.apply_rolling_avg = false;
    
    core::Processor processor(config);
    
    io::ReaderCSV reader({"data/air.csv"});
    auto batch = reader.next_batch(100);
    processor.process_batch(batch);
    
    auto stats = processor.get_bucket_stats();
    ASSERT_TRUE(stats.size() > 0); // Should create at least one bucket
}

TEST(processor_computes_mean) {
    core::Processor::Config config;
    config.bucket_minutes = 60; // Large bucket to get all data
    config.apply_rolling_avg = false;
    
    core::Processor processor(config);
    
    io::ReaderCSV reader({"data/air.csv"});
    auto batch = reader.next_batch(100);
    processor.process_batch(batch);
    
    auto stats = processor.get_bucket_stats();
    ASSERT_TRUE(stats.size() > 0);
    
    // Check that mean is calculated
    bool found_mean = false;
    for (const auto& bucket : stats) {
        if (bucket.pm25_mean.has_value()) {
            ASSERT_TRUE(bucket.pm25_mean.value() > 0);
            found_mean = true;
        }
    }
    ASSERT_TRUE(found_mean);
}

TEST(processor_computes_percentiles) {
    core::Processor::Config config;
    config.bucket_minutes = 60;
    config.apply_rolling_avg = false;
    
    core::Processor processor(config);
    
    io::ReaderCSV reader({"data/air.csv"});
    auto batch = reader.next_batch(100);
    processor.process_batch(batch);
    
    auto stats = processor.get_bucket_stats();
    
    bool found_percentiles = false;
    for (const auto& bucket : stats) {
        if (bucket.pm25_p90.has_value() && bucket.pm25_p99.has_value()) {
            // P99 should be >= P90
            ASSERT_TRUE(bucket.pm25_p99.value() >= bucket.pm25_p90.value());
            found_percentiles = true;
        }
    }
    ASSERT_TRUE(found_percentiles);
}

TEST(processor_applies_rolling_average) {
    core::Processor::Config config;
    config.bucket_minutes = 1; // Small buckets
    config.rolling_window_size = 3;
    config.apply_rolling_avg = true;
    
    core::Processor processor(config);
    
    io::ReaderCSV reader({"data/traffic.csv"});
    auto batch = reader.next_batch(100);
    processor.process_batch(batch);
    
    auto diag = processor.get_diagnostics();
    ASSERT_TRUE(diag.processed > 0);
}

TEST(processor_handles_multiple_sensors) {
    core::Processor::Config config;
    config.bucket_minutes = 60;
    config.apply_rolling_avg = false;
    
    core::Processor processor(config);
    
    // Process all three sensor types
    io::ReaderCSV reader({"data/air.csv", "data/noise.csv", "data/traffic.csv"});
    auto batch = reader.next_batch(100);
    processor.process_batch(batch);
    
    auto stats = processor.get_bucket_stats();
    
    // Should have metrics from all sensor families
    bool has_air = false, has_noise = false, has_traffic = false;
    for (const auto& bucket : stats) {
        if (bucket.pm25_mean.has_value()) has_air = true;
        if (bucket.db_mean.has_value()) has_noise = true;
        if (bucket.speed_mean.has_value()) has_traffic = true;
    }
    
    ASSERT_TRUE(has_air);
    ASSERT_TRUE(has_noise);
    ASSERT_TRUE(has_traffic);
}

TEST(processor_groups_by_zone_and_time) {
    core::Processor::Config config;
    config.bucket_minutes = 5;
    config.apply_rolling_avg = false;
    
    core::Processor processor(config);
    
    io::ReaderCSV reader({"data/air.csv"});
    auto batch = reader.next_batch(100);
    processor.process_batch(batch);
    
    auto stats = processor.get_bucket_stats();
    
    // Should have multiple buckets (different zones/times)
    ASSERT_TRUE(stats.size() >= 2);
}

// ============= Integration Tests =============

TEST(integration_full_pipeline) {
    // Load config
    io::ReaderJSON json_reader("data/config.json");
    auto config = json_reader.parse_config();
    
    // Setup processor
    core::Processor::Config proc_config;
    proc_config.bucket_minutes = config.window_minutes;
    proc_config.apply_rolling_avg = true;
    
    core::Processor processor(proc_config);
    
    // Read and process
    io::ReaderCSV csv_reader(config.inputs);
    auto batch = csv_reader.next_batch(100);
    processor.process_batch(batch);
    
    // Verify results
    auto stats = processor.get_bucket_stats();
    ASSERT_TRUE(stats.size() > 0);
    
    auto diag = processor.get_diagnostics();
    ASSERT_TRUE(diag.processed > 0);
}

TEST(integration_cli_overrides_config) {
    io::ReaderJSON json_reader("data/config.json");
    auto config = json_reader.parse_config();
    
    // Simulate CLI override
    config.inputs = {"data/traffic.csv"};
    config.window_minutes = 10;
    
    core::Processor::Config proc_config;
    proc_config.bucket_minutes = config.window_minutes;
    proc_config.zone_filter = {1};
    
    core::Processor processor(proc_config);
    
    io::ReaderCSV csv_reader(config.inputs);
    auto batch = csv_reader.next_batch(100);
    processor.process_batch(batch);
    
    auto diag = processor.get_diagnostics();
    ASSERT_TRUE(diag.filtered_by_zone > 0); // Zone filter applied
}

// ============= Main Test Runner =============

int main() {
    std::cout << "========================================\n";
    std::cout << "CitySense Test Suite\n";
    std::cout << "========================================\n\n";
    
    std::cout << "--- ReaderCSV Tests ---\n";
    run_test_csv_reads_air_data();
    run_test_csv_reads_noise_data();
    run_test_csv_reads_traffic_data();
    run_test_csv_handles_multiple_files();
    run_test_csv_tracks_parse_stats();
    run_test_csv_parses_timestamps();
    run_test_csv_case_insensitive_headers();
    
    std::cout << "\n--- ReaderJSON Tests ---\n";
    run_test_json_reads_config();
    run_test_json_parses_inputs_array();
    run_test_json_uses_defaults();
    
    std::cout << "\n--- Processor Tests ---\n";
    run_test_processor_filters_by_zone();
    run_test_processor_creates_buckets();
    run_test_processor_computes_mean();
    run_test_processor_computes_percentiles();
    run_test_processor_applies_rolling_average();
    run_test_processor_handles_multiple_sensors();
    run_test_processor_groups_by_zone_and_time();
    
    std::cout << "\n--- Integration Tests ---\n";
    run_test_integration_full_pipeline();
    run_test_integration_cli_overrides_config();
    
    std::cout << "\n========================================\n";
    std::cout << "Test Results:\n";
    std::cout << "  PASSED: " << tests_passed << "\n";
    std::cout << "  FAILED: " << tests_failed << "\n";
    std::cout << "  TOTAL:  " << (tests_passed + tests_failed) << "\n";
    std::cout << "========================================\n";
    
    return tests_failed == 0 ? 0 : 1;
}
