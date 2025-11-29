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
    else { // Simulator
        using namespace std::chrono;
        system_clock::time_point start{ seconds{0} };
        sim::Clock      clock{ start, 60 };         // 60s step
        sim::SeededRNG  rng{ opt.sim_seed };
        sim::Simulator  sim{ clock, rng };
        sim.start(sim::SimulatorProfile::Weekday);  // can be made configurable later

        while (true) {
            auto batch = sim.next_batch(static_cast<int>(opt.batch_size));
            if (batch.empty()) break;
            for (auto& rec : batch) {
                accept_record(rec);
            }
        }
    }

    // Obtain summary for Role B exporters.
    auto sum = agg.summary();
    (void)sum; // avoid unused-variable warning until exporters are wired

    return 0;
}
