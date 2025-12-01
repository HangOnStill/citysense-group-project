#include <iostream>
#include <iomanip>
#include "src/io/ReaderCSV.hpp"
#include "src/io/ReaderJSON.hpp"
#include "src/core/Processor.hpp"
#include "src/app/Cli.hpp"

std::string format_time(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm;
    #ifdef _WIN32
    localtime_s(&tm, &time_t);
    #else
    localtime_r(&time_t, &tm);
    #endif
    
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buffer);
}

void print_bucket_stats(const std::vector<core::BucketStats>& stats) {
    std::cout << "\n========================================\n";
    std::cout << "BUCKET STATISTICS (Aggregated Results)\n";
    std::cout << "========================================\n";
    
    for (const auto& bucket : stats) {
        std::cout << "\n--- Bucket: " << format_time(bucket.bucket_start) 
                  << " | Zone: " << bucket.zone_id 
                  << " | Count: " << bucket.count << " ---\n";
        
        // Traffic metrics
        if (bucket.speed_mean.has_value()) {
            std::cout << "  Speed (km/h):\n";
            std::cout << "    Mean: " << std::fixed << std::setprecision(2) 
                      << bucket.speed_mean.value() << "\n";
            if (bucket.speed_median.has_value())
                std::cout << "    Median: " << bucket.speed_median.value() << "\n";
            if (bucket.speed_p90.has_value())
                std::cout << "    P90: " << bucket.speed_p90.value() << "\n";
            if (bucket.speed_p99.has_value())
                std::cout << "    P99: " << bucket.speed_p99.value() << "\n";
        }
        
        if (bucket.flow_mean.has_value()) {
            std::cout << "  Flow (per min):\n";
            std::cout << "    Mean: " << bucket.flow_mean.value() << "\n";
            if (bucket.flow_median.has_value())
                std::cout << "    Median: " << bucket.flow_median.value() << "\n";
            if (bucket.flow_p90.has_value())
                std::cout << "    P90: " << bucket.flow_p90.value() << "\n";
            if (bucket.flow_p99.has_value())
                std::cout << "    P99: " << bucket.flow_p99.value() << "\n";
        }
        
        // Air quality metrics
        if (bucket.pm25_mean.has_value()) {
            std::cout << "  PM2.5 (µg/m³):\n";
            std::cout << "    Mean: " << bucket.pm25_mean.value() << "\n";
            if (bucket.pm25_median.has_value())
                std::cout << "    Median: " << bucket.pm25_median.value() << "\n";
            if (bucket.pm25_p90.has_value())
                std::cout << "    P90: " << bucket.pm25_p90.value() << "\n";
            if (bucket.pm25_p99.has_value())
                std::cout << "    P99: " << bucket.pm25_p99.value() << "\n";
        }
        
        if (bucket.pm10_mean.has_value()) {
            std::cout << "  PM10 (µg/m³):\n";
            std::cout << "    Mean: " << bucket.pm10_mean.value() << "\n";
            if (bucket.pm10_median.has_value())
                std::cout << "    Median: " << bucket.pm10_median.value() << "\n";
            if (bucket.pm10_p90.has_value())
                std::cout << "    P90: " << bucket.pm10_p90.value() << "\n";
            if (bucket.pm10_p99.has_value())
                std::cout << "    P99: " << bucket.pm10_p99.value() << "\n";
        }
        
        // Noise metrics
        if (bucket.db_mean.has_value()) {
            std::cout << "  Noise (dB):\n";
            std::cout << "    Mean: " << bucket.db_mean.value() << "\n";
            if (bucket.db_median.has_value())
                std::cout << "    Median: " << bucket.db_median.value() << "\n";
            if (bucket.db_p90.has_value())
                std::cout << "    P90: " << bucket.db_p90.value() << "\n";
            if (bucket.db_p99.has_value())
                std::cout << "    P99: " << bucket.db_p99.value() << "\n";
        }
    }
}

int main(int argc, char** argv) {
    std::cout << "======================================\n";
    std::cout << "CitySense - Urban Sensor Data Pipeline\n";
    std::cout << "======================================\n\n";
    
    // Parse CLI arguments
    auto cli_args = app::parse_cli(argc, argv);
    
    if (cli_args.help) {
        app::print_usage(argv[0]);
        return 0;
    }
    
    try {
        // Load configuration
        std::cout << "Loading configuration from: " << cli_args.config_path << "\n";
        io::ReaderJSON json_reader(cli_args.config_path);
        auto config = json_reader.parse_config();
        
        // Override with CLI args if provided
        if (!cli_args.inputs.empty()) {
            config.inputs = cli_args.inputs;
        }
        if (cli_args.window_minutes.has_value()) {
            config.window_minutes = cli_args.window_minutes.value();
        }
        if (cli_args.out_path.has_value()) {
            config.out_path = cli_args.out_path.value();
        }
        
        std::cout << "Configuration:\n";
        std::cout << "  Input files: ";
        for (const auto& input : config.inputs) {
            std::cout << input << " ";
        }
        std::cout << "\n  Window: " << config.window_minutes << " minutes\n";
        std::cout << "  Output: " << config.out_path << "\n";
        
        if (!cli_args.zones.empty()) {
            std::cout << "  Zone filter: ";
            for (auto z : cli_args.zones) {
                std::cout << z << " ";
            }
            std::cout << "\n";
        }
        
        // Setup processor
        core::Processor::Config proc_config;
        proc_config.zone_filter = cli_args.zones;
        proc_config.bucket_minutes = config.window_minutes;
        proc_config.rolling_window_size = 3;  // 3-reading rolling average
        proc_config.apply_rolling_avg = true;
        
        core::Processor processor(proc_config);
        
        // Read and process CSV files
        std::cout << "\n--- Reading CSV Files ---\n";
        io::ReaderCSV csv_reader(config.inputs);
        
        size_t total_read = 0;
        while (true) {
            auto batch = csv_reader.next_batch(100);
            if (batch.empty()) break;
            
            total_read += batch.size();
            processor.process_batch(batch);
        }
        
        // Display CSV parsing stats
        auto csv_stats = csv_reader.get_stats();
        std::cout << "CSV Parse Statistics:\n";
        std::cout << "  Total rows: " << csv_stats.total_rows << "\n";
        std::cout << "  Successfully parsed: " << csv_stats.parsed_rows << "\n";
        std::cout << "  Malformed rows: " << csv_stats.malformed_rows << "\n";
        
        if (!csv_stats.errors.empty()) {
            std::cout << "  Errors:\n";
            for (size_t i = 0; i < std::min(csv_stats.errors.size(), size_t(5)); ++i) {
                std::cout << "    - " << csv_stats.errors[i] << "\n";
            }
        }
        
        // Display processing diagnostics
        auto proc_diag = processor.get_diagnostics();
        std::cout << "\n--- Processing Pipeline Diagnostics ---\n";
        std::cout << "  Total records received: " << proc_diag.total_records << "\n";
        std::cout << "  Filtered by time: " << proc_diag.filtered_by_time << "\n";
        std::cout << "  Filtered by zone: " << proc_diag.filtered_by_zone << "\n";
        std::cout << "  Successfully processed: " << proc_diag.processed << "\n";
        
        // Get and display aggregated statistics
        auto bucket_stats = processor.get_bucket_stats();
        print_bucket_stats(bucket_stats);
        
        std::cout << "\n========================================\n";
        std::cout << "Processing complete! " << bucket_stats.size() << " buckets generated.\n";
        std::cout << "========================================\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
}
