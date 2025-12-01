// dev/test_readerjson_full.cpp
#include <iostream>
#include <string>

#include "io/ReaderJSON.hpp"
#include "app/Config.hpp"

int main() {
    std::cout << "=== Testing ReaderJSON (full config demo) ===\n";

    try {
        // Test 1: Create reader
        std::cout << "\n1. Creating ReaderJSON...\n";
        io::ReaderJSON reader("data/config.json");
        std::cout << "   Reader created successfully.\n";

        // Test 2: Read file
        std::cout << "\n2. Reading JSON file...\n";
        std::string content = reader.read_file();
        std::cout << "   File read successfully.\n";
        std::cout << "   Content:\n" << content << "\n";

        // Test 3: Parse config
        std::cout << "\n3. Parsing config...\n";
        app::Config config = reader.parse_config();
        std::cout << "   Config parsed successfully.\n";

        // Test 4: Display all parsed values
        std::cout << "\n4. Parsed Config Values:\n";
        std::cout << "   Inputs (" << config.inputs.size() << "): ";
        for (const auto& input : config.inputs) {
            std::cout << input << ' ';
        }
        std::cout << "\n";

        std::cout << "   out_path: " << config.out_path << "\n";
        std::cout << "   window_minutes: " << config.window_minutes << "\n";
        std::cout << "   time_step_seconds: " << config.time_step_seconds << "\n";
        std::cout << "   report_every_ticks: " << config.report_every_ticks << "\n";
        std::cout << "   traffic_speed_threshold: " << config.traffic_speed_threshold << "\n";
        std::cout << "   consec_minutes: " << config.consec_minutes << "\n";
        std::cout << "   pm25_threshold: " << config.pm25_threshold << "\n";
        std::cout << "   db_threshold: " << config.db_threshold << "\n";
        std::cout << "   noise_count_threshold: " << config.noise_count_threshold << "\n";
        std::cout << "   noise_window_minutes: " << config.noise_window_minutes << "\n";

        // Test 5: Test trim
        std::cout << "\n5. Testing trim() function...\n";
        std::string test1 = reader.trim("  hello  ");
        std::string test2 = reader.trim("world");
        std::string test3 = reader.trim("   ");

        std::cout << "   Input: '  hello  ' -> Output: '" << test1 << "'\n";
        std::cout << "   Input: 'world'     -> Output: '" << test2 << "'\n";
        std::cout << "   Input: '   '       -> Output: '" << test3 << "'\n";

        if (test1 == "hello" && test2 == "world" && test3.empty()) {
            std::cout << "   All trim() tests passed.\n";
        }
        else {
            std::cout << "   Trim tests failed.\n";
            return 1;
        }

        std::cout << "\nAll ReaderJSON tests completed successfully.\n";
    }
    catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
