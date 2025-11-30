#include <iostream>
#include "app/Options.hpp"
#include "app/Options.cpp"
#include "io/ReaderCSV.hpp"
#include "sim/Simulator.hpp"
#include "sim/Simulator.cpp"
#include "sim/SeededRNG.hpp"
#include "sim/Clock.hpp"
#include "core/Aggregator.hpp"

int main(int argc, char** argv) {
    app::Options opt;
    try {
        opt = app::parse_args(argc, argv);
    }
    catch (const std::exception& ex) {
        std::cerr << "Argument error: " << ex.what() << "\n";
        return 1;
    }

    // Aggregator now has a default window size; you can override if you want, e.g. Aggregator agg{5};


    core::Aggregator agg(1);



    if (opt.reserve_rows > 0) {
        agg.reserve(opt.reserve_rows);
    }

    auto accept_record = [&](const model::SensorRecord& rec) {
        // zone filter
        if (!opt.zones.empty()) {
            bool ok = false;
            for (int z : opt.zones) {
                if (rec.zone_id == z) { ok = true; break; }
            }
            if (!ok) return;
        }
        // time filters
        if (opt.from && rec.ts < *opt.from) return;
        if (opt.to && rec.ts >= *opt.to)  return;

        agg.consume(rec);
        };

    if (opt.mode == app::IngestMode::Csv) {
        io::ReaderCSV reader{ opt.inputs };
        while (true) {
            auto batch = reader.next_batch(opt.batch_size);
            if (batch.empty()) break;
            for (auto& rec : batch) {
                accept_record(rec);
            }
        }
    }
    else { // Simulator mode
    using namespace std::chrono;

    // 1. Determine simulation start time
    system_clock::time_point start;

    if (opt.from) {
        start = *opt.from;
    } else {
        // fallback default start: now
        start = system_clock::now();
    }

    // 2. Create clock with configured step (seconds per simulated minute)
    int step_seconds = 60;  // you already use 60 in Simulator
    sim::Clock clock{ start, step_seconds };

    // 3. Create RNG and simulator
    sim::SeededRNG rng{ opt.sim_seed };
    sim::Simulator sim{ clock, rng };
    sim.start(sim::SimulatorProfile::Weekday);

    // 4. Compute end time using --hours
    system_clock::time_point end = start + hours(opt.sim_hours);

    // 5. Simulation loop (will stop correctly now)
    while (clock.now() < end) {
        auto batch = sim.next_batch(static_cast<int>(opt.batch_size));
        for (auto& rec : batch)
            accept_record(rec);
    }
}
    // Obtain summary for Role B exporters.
    auto sum = agg.summary();
    (void)sum; // avoid unused-variable warning until exporters are wired

    return 0;
}
