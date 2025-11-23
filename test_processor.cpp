#include <iostream>
#include "src/core/Processor.hpp"
#include "src/io/ReaderCSV.hpp"

using namespace std;

int main() {
    try {
        cout << "=== Testing Processor ===" << endl;
        
        // Step 1: Read sensor data
        cout << "\n1. Reading CSV data..." << endl;
        io::ReaderCSV reader({"data/air.csv"});
        auto records = reader.next_batch(100);
        cout << "   ✅ Read " << records.size() << " records" << endl;
        
        // Step 2: Configure processor
        cout << "\n2. Configuring processor..." << endl;
        core::Processor::Config config;
        config.bucket_minutes = 5;
        config.rolling_window_size = 3;
        config.apply_rolling_avg = true;
        // No filters = process all data
        cout << "   ✅ Processor configured" << endl;
        
        // Step 3: Process the batch
        cout << "\n3. Processing batch..." << endl;
        core::Processor processor(config);
        processor.process_batch(records);
        cout << "   ✅ Batch processed" << endl;
        
        // Step 4: Get diagnostics
        cout << "\n4. Diagnostics:" << endl;
        auto diag = processor.get_diagnostics();
        cout << "   Total records: " << diag.total_records << endl;
        cout << "   Filtered out: " << diag.filtered_out << endl;
        cout << "   Processed: " << diag.processed << endl;
        
        // Step 5: Get bucket statistics
        cout << "\n5. Bucket Statistics:" << endl;
        auto stats = processor.get_bucket_stats();
        cout << "   Total buckets: " << stats.size() << endl;
        
        for (size_t i = 0; i < min(stats.size(), size_t(3)); ++i) {
            const auto& bucket = stats[i];
            cout << "\n   Bucket " << (i+1) << ":" << endl;
            cout << "     Zone: " << bucket.zone_id << endl;
            cout << "     Count: " << bucket.count << endl;
            if (bucket.pm25_mean.has_value()) {
                cout << "     PM2.5 Mean: " << bucket.pm25_mean.value() << endl;
            }
            if (bucket.pm25_median.has_value()) {
                cout << "     PM2.5 Median: " << bucket.pm25_median.value() << endl;
            }
            if (bucket.pm10_mean.has_value()) {
                cout << "     PM10 Mean: " << bucket.pm10_mean.value() << endl;
            }
        }
        
        cout << "\n🎉 All processor tests passed!" << endl;
        
    } catch (const exception& e) {
        cerr << "❌ Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
