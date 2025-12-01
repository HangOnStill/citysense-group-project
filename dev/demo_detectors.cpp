// dev/demo_detectors.cpp
#include <iostream>
#include <chrono>

#include "core/Window.hpp"
#include "core/HighNoiseDetector.hpp"
#include "model/SensorRecord.hpp"

int main() {
    using namespace std;
    using namespace std::chrono;

    cout << "=== demo_detectors (HighNoiseDetector) ===\n\n";

    core::Window w;

    // Synthetic window: 3 noise records, one above the threshold
    auto t0 = system_clock::now();

    model::SensorRecord r1{};
    r1.ts = t0;
    r1.zone_id = 1;
    r1.sensor_id = "noise-1";
    r1.db = 65.0;

    model::SensorRecord r2{};
    r2.ts = t0 + minutes(1);
    r2.zone_id = 2;
    r2.sensor_id = "noise-2";
    r2.db = 78.0;

    model::SensorRecord r3{};
    r3.ts = t0 + minutes(2);
    r3.zone_id = 3;
    r3.sensor_id = "noise-3";
    r3.db = 90.0; // this one should trigger

    w.add_record(r1);
    w.add_record(r2);
    w.add_record(r3);

    core::HighNoiseDetector det(80.0);
    auto findings = det.detect(w);

    if (findings.empty()) {
        cout << "No high-noise findings in this window.\n";
    }
    else {
        cout << "Findings:\n";
        for (const auto& f : findings) {
            cout << "  [" << f.detector << "] "
                << f.type << " in zone " << f.zone_id
                << " at sensor " << f.sensor_id << "\n";
            cout << "    message: " << f.message << "\n";
            cout << "    value:   " << f.value << " dB\n";
            auto it = f.thresholds.find("db");
            if (it != f.thresholds.end()) {
                cout << "    threshold: " << it->second << " dB\n";
            }
        }
    }

    cout << "\n(This demonstrates that detectors are real, operate on Window data,\n"
        " and emit structured core::Finding objects.)\n";

    return 0;
}
