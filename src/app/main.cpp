// src/app/main.cpp
#include <chrono>
#include <exception>
#include <iostream>

#include "app/Options.hpp"
#include "core/Aggregator.hpp"
#include "io/ReaderCSV.hpp"
#include "model/SensorRecord.hpp"
#include "sim/Clock.hpp"
#include "sim/SeededRNG.hpp"
#include "sim/Simulator.hpp"
#include "sim/SimulatorProfile.hpp"

int main(int argc, char** argv) {
    using std::cerr;
    using std::cout;
    using namespace std::chrono;

    // ------------------------------------------------------------
    // 1. Parse CLI options
    // ------------------------------------------------------------
    app::Options opt;
    try {
        opt = app::parse_args(argc, argv);
    }
    catch (const std::exception& ex) {
        cerr << "Argument error: " << ex.what() << '\n';
        return 1;
    }

    // ------------------------------------------------------------
    // 2. Aggregator: 1-minute window, optional pre-reserve
    // ------------------------------------------------------------
    core::Aggregator agg{ 1 };
    if (opt.reserve_rows > 0) {
        agg.reserve(opt.reserve_rows);
    }

    // Filtering + ingest hook used by both CSV and simulator modes
    auto accept_record = [&](const model::SensorRecord& rec) {
        // Zone filter
        if (!opt.zones.empty()) {
            bool in_zone = false;
            for (int z : opt.zones) {
                if (rec.zone_id == z) {
                    in_zone = true;
                    break;
                }
            }
            if (!in_zone) return;
        }

        // Time filters
        if (opt.from && rec.ts < *opt.from) return;
        if (opt.to && rec.ts >= *opt.to) return;

        agg.consume(rec);
        };

    // ------------------------------------------------------------
    // 3. Ingestion: CSV mode or Simulator mode
    // ------------------------------------------------------------
    try {
        if (opt.mode == app::IngestMode::Csv) {
            // -------- CSV ingestion --------
            io::ReaderCSV reader{ opt.inputs };

            for (;;) {
                auto batch = reader.next_batch(opt.batch_size);
                if (batch.empty()) break;

                for (auto& rec : batch) {
                    accept_record(rec);
                }
            }

        }
        else {
            // -------- Simulator ingestion --------
            system_clock::time_point start = opt.from.value_or(system_clock::now());

            // 60-second simulation step
            int step_seconds = 60;
            sim::Clock clock{ start, step_seconds };
            sim::SeededRNG rng{ opt.sim_seed };
            sim::Simulator sim{ clock, rng };
            sim.start(sim::SimulatorProfile::Weekday);

            // For now: run a fixed amount of simulated time (e.g., 60 steps = 1h)
            std::size_t max_steps = 60;

            while (max_steps-- > 0) {
                auto batch = sim.next_batch(static_cast<int>(opt.batch_size));
                if (batch.empty()) break;

                for (auto& rec : batch) {
                    accept_record(rec);
                }
            }
        }
    }
    catch (const std::exception& ex) {
        cerr << "Runtime error: " << ex.what() << '\n';
        return 2;
    }

    // ------------------------------------------------------------
    // 4. Final summary to stdout
    // ------------------------------------------------------------
    auto sum = agg.summary();
    cout << "Total ingested rows: " << sum.total_count << "\n";
    cout << "Per zone:\n";
    for (auto& [zone, count] : sum.by_zone) {
        cout << "  zone " << zone << ": " << count << "\n";
    }

    return 0;
}
