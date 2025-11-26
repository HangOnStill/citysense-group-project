#include <iostream>
#include "src/io/ReaderCSV.hpp"
#include "src/io/ReaderJSON.hpp"

int main() {
    std::cout << "=== Testing ReaderCSV ===" << std::endl;
    
    try {
        // Test air.csv
        io::ReaderCSV air_reader({"data/air.csv"});
        auto air_batch = air_reader.next_batch(5);
        
        std::cout << "Air CSV Records: " << air_batch.size() << std::endl;
        for (const auto& record : air_batch) {
            std::cout << "  Sensor: " << record.sensor_id 
                      << ", Zone: " << record.zone_id;
            if (record.pm25.has_value()) {
                std::cout << ", PM2.5: " << record.pm25.value();
            }
            if (record.pm10.has_value()) {
                std::cout << ", PM10: " << record.pm10.value();
            }
            std::cout << std::endl;
        }
        
        // Test noise.csv
        io::ReaderCSV noise_reader({"data/noise.csv"});
        auto noise_batch = noise_reader.next_batch(5);
        
        std::cout << "\nNoise CSV Records: " << noise_batch.size() << std::endl;
        for (const auto& record : noise_batch) {
            std::cout << "  Sensor: " << record.sensor_id 
                      << ", Zone: " << record.zone_id;
            if (record.db.has_value()) {
                std::cout << ", dB: " << record.db.value();
            }
            std::cout << std::endl;
        }
        
        // Test traffic.csv
        io::ReaderCSV traffic_reader({"data/traffic.csv"});
        auto traffic_batch = traffic_reader.next_batch(5);
        
        std::cout << "\nTraffic CSV Records: " << traffic_batch.size() << std::endl;
        for (const auto& record : traffic_batch) {
            std::cout << "  Sensor: " << record.sensor_id 
                      << ", Zone: " << record.zone_id;
            if (record.speed.has_value()) {
                std::cout << ", Speed: " << record.speed.value();
            }
            if (record.flow.has_value()) {
                std::cout << ", Flow: " << record.flow.value();
            }
            std::cout << std::endl;
        }
        
        // Print statistics
        auto stats = traffic_reader.get_stats();
        std::cout << "\nParse Statistics:" << std::endl;
        std::cout << "  Total rows: " << stats.total_rows << std::endl;
        std::cout << "  Parsed rows: " << stats.parsed_rows << std::endl;
        std::cout << "  Malformed rows: " << stats.malformed_rows << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "CSV Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== Testing ReaderJSON ===" << std::endl;
    
    try {
        io::ReaderJSON json_reader("data/config.json");
        auto config = json_reader.parse_config();
        
        std::cout << "Config loaded successfully:" << std::endl;
        std::cout << "  Inputs: ";
        for (const auto& input : config.inputs) {
            std::cout << input << " ";
        }
        std::cout << std::endl;
        std::cout << "  Window minutes: " << config.window_minutes << std::endl;
        std::cout << "  Output path: " << config.out_path << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "JSON Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
    return 0;
}
