#include <iostream>
#include "io/ReaderJSON.hpp"

using namespace std;

int main() {
    cout << "=== Testing ReaderJSON ===" << endl;
    
    try {
        // Test 1: Create reader
        cout << "\n1. Creating ReaderJSON..." << endl;
        io::ReaderJSON reader("data/config.json");
        cout << "   ✅ Reader created successfully" << endl;
        
        // Test 2: Read file
        cout << "\n2. Reading JSON file..." << endl;
        string content = reader.read_file();
        cout << "   ✅ File read successfully" << endl;
        cout << "   Content:\n" << content << endl;
        
        // Test 3: Parse config
        cout << "\n3. Parsing config..." << endl;
        app::Config config = reader.parse_config();
        cout << "   ✅ Config parsed successfully" << endl;
        
        // Test 4: Display all parsed values
        cout << "\n4. Parsed Config Values:" << endl;
        cout << "   Inputs (" << config.inputs.size() << "): ";
        for (const auto& input : config.inputs) {
            cout << input << " ";
        }
        cout << endl;
        cout << "   out_path: " << config.out_path << endl;
        cout << "   window_minutes: " << config.window_minutes << endl;
        cout << "   time_step_seconds: " << config.time_step_seconds << endl;
        cout << "   report_every_ticks: " << config.report_every_ticks << endl;
        cout << "   traffic_speed_threshold: " << config.traffic_speed_threshold << endl;
        cout << "   consec_minutes: " << config.consec_minutes << endl;
        cout << "   pm25_threshold: " << config.pm25_threshold << endl;
        cout << "   db_threshold: " << config.db_threshold << endl;
        cout << "   noise_count_threshold: " << config.noise_count_threshold << endl;
        cout << "   noise_window_minutes: " << config.noise_window_minutes << endl;
        
        // Test 5: Test trim
        cout << "\n5. Testing trim() function..." << endl;
        string test1 = reader.trim("  hello  ");
        string test2 = reader.trim("world");
        string test3 = reader.trim("   ");
        
        cout << "   Input: '  hello  ' → Output: '" << test1 << "'" << endl;
        cout << "   Input: 'world'     → Output: '" << test2 << "'" << endl;
        cout << "   Input: '   '       → Output: '" << test3 << "'" << endl;
        
        if (test1 == "hello" && test2 == "world" && test3 == "") {
            cout << "   ✅ All trim tests passed!" << endl;
        } else {
            cout << "   ❌ Trim tests failed!" << endl;
            return 1;
        }
        
        cout << "\n🎉 All tests passed successfully!" << endl;
        
    } catch (const exception& e) {
        cerr << "\n❌ Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
