// src/app/main.cpp
#include <iostream>
#include <exception>

#include "app/Options.hpp"
#include "io/ReaderCSV.hpp"
#include "sim/Simulator.hpp"
#include "sim/SeededRNG.hpp"
#include "sim/Clock.hpp"
#include "core/Aggregator.hpp"
#include "model/SensorRecord.hpp"

int main(int argc, char** argv) {
    app::Options opt;
    try {
        opt = app::parse_args(argc, argv);
    }
    catch (const std::exception& ex) {
        std::cerr << "Argument error: " << ex.what() << "\n";
        return 1;
    }

    // 1-minute time window (you can tune this if needed)
    core::Aggregator agg{ 1 };
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
    else {
        using namespace std::chrono;
        system_clock::time_point start{ seconds{0} };
        sim::Clock     clock{ start, 60 };           // 60s step
        sim::SeededRNG rng{ opt.sim_seed };
        sim::Simulator sim{ clock, rng };
        sim.start(sim::SimulatorProfile::Weekday);   // could be made configurable

        while (true) {
            auto batch = sim.next_batch(static_cast<int>(opt.batch_size));
            if (batch.empty()) break;
            for (auto& rec : batch) {
                accept_record(rec);
            }
        }
    }

    auto sum = agg.summary();
    std::cout << "Total ingested rows: " << sum.total_count << "\n";
    std::cout << "Per zone:\n";
    for (auto& [zone, count] : sum.by_zone) {
        std::cout << "  zone " << zone << ": " << count << "\n";
    }

    return 0;
}
