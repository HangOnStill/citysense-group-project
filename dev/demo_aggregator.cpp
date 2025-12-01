// dev/demo_aggregator.cpp
#include <iostream>
#include <chrono>
#include <map>
#include <vector>
#include <algorithm>
#include <numeric>

#include "core/Aggregator.hpp"
#include "model/SensorRecord.hpp"

int main() {
    using namespace std;
    using namespace std::chrono;

    cout << "=== demo_aggregator (synthetic example) ===\n\n";

    // 1-minute window aggregator (same window size used by the main app)
    core::Aggregator agg{ 1 };

    auto t0 = system_clock::now();

    // -----------------------------------------------------------------
    // Construct synthetic SensorRecord rows
    //
    // Zone 1: speeds 30, 40   -> mean = 35, p90 = 40
    // Zone 2: speeds 45, 55   -> mean = 50, p90 = 55
    // -----------------------------------------------------------------
    model::SensorRecord r1{};
    r1.ts = t0;
    r1.zone_id = 1;
    r1.sensor_id = "demo-traffic-1";
    r1.speed = 30.0;
    r1.flow = 10.0;

    model::SensorRecord r2{};
    r2.ts = t0 + minutes(1);
    r2.zone_id = 1;
    r2.sensor_id = "demo-traffic-2";
    r2.speed = 40.0;
    r2.flow = 20.0;

    model::SensorRecord r3{};
    r3.ts = t0 + minutes(2);
    r3.zone_id = 2;
    r3.sensor_id = "demo-traffic-3";
    r3.speed = 45.0;
    r3.flow = 12.0;

    model::SensorRecord r4{};
    r4.ts = t0 + minutes(3);
    r4.zone_id = 2;
    r4.sensor_id = "demo-traffic-4";
    r4.speed = 55.0;
    r4.flow = 18.0;

    vector<model::SensorRecord> all_recs{ r1, r2, r3, r4 };

    // Feed into aggregator (Range-style consume that matches your tests)
    agg.consume(all_recs);

    // Also keep speeds per zone locally for the demo statistics
    map<int, vector<double>> zone_speeds;
    for (const auto& rec : all_recs) {
        if (rec.speed) {
            zone_speeds[rec.zone_id].push_back(*rec.speed);
        }
    }

    // -----------------------------------------------------------------
    // Aggregator summary (counts)
    // -----------------------------------------------------------------
    auto sum = agg.summary();

    cout << "Total ingested rows: " << sum.total_count << "\n";
    cout << "Per zone (record counts):\n";
    for (const auto& [zone, count] : sum.by_zone) {
        cout << "  zone " << zone << ": " << count << " records\n";
    }
    cout << "\n";

    // -----------------------------------------------------------------
    // Per-zone speed statistics (mean and p90) for the demo
    // -----------------------------------------------------------------
    cout << "Per zone speed statistics (computed on synthetic data):\n";

    for (auto& [zone, speeds] : zone_speeds) {
        if (speeds.empty()) {
            continue;
        }

        sort(speeds.begin(), speeds.end());

        // mean
        double mean = accumulate(speeds.begin(), speeds.end(), 0.0) /
            static_cast<double>(speeds.size());

        // simple p90: for this small demo, just take the last element
        // (with 2 records per zone, that gives the higher speed as p90)
        double p90 = speeds.back();

        cout << "  Zone " << zone
            << " (" << speeds.size() << " records): "
            << "mean speed: " << mean << " km/h, "
            << "p90 speed: " << p90 << " km/h\n";
    }

    cout << "\n(For the full pipeline, use citysense.exe: "
        "CSV/Simulator -> Aggregator -> detectors.)\n";

    return 0;
}
