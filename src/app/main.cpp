// src/app/main.cpp
#include <iostream>
#include <exception>
#include <chrono>

#include "app/Options.hpp"
#include "io/ReaderCSV.hpp"
#include "sim/Simulator.hpp"
#include "sim/PatternAnalytics.hpp"
#include "sim/Criteria.hpp"
#include "sim/SeededRNG.hpp"
#include "sim/Clock.hpp"
#include "core/Aggregator.hpp"
#include "model/SensorRecord.hpp"

int main(int argc, char** argv) {
    app::Options opt;
    try {
        opt = app::parse_args(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << "Argument error: " << ex.what() << "\n";
        return 1;
    }

    // 1-minute window (tune if required)
    core::Aggregator agg{1};
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
        if (opt.to   && rec.ts >= *opt.to)  return;

        agg.consume(rec);
    };

    if (opt.mode == app::IngestMode::Csv) {
        // CSV ingest mode
        io::ReaderCSV reader{ opt.inputs };
        for (;;) {
            auto batch = reader.next_batch(opt.batch_size);
            if (batch.empty()) break;
            for (auto& rec : batch) {
                accept_record(rec);
            }
        }
    } else {
        // Simulator mode
        using namespace std::chrono;

        // 1. Determine simulation start time
        system_clock::time_point start;
        if (opt.from) {
            start = *opt.from;
        } else {
            // fallback default start: now
            start = system_clock::now();
        }

        // 2. Create clock with 60-second step
        int step_seconds = 60;
        sim::Clock clock{ start, step_seconds };

        // 3. RNG + simulator
        sim::SeededRNG rng{ opt.sim_seed };
        sim::Simulator sim{ clock, rng };

        if (opt.run_patterns) {
            // Pattern analytics branch (no aggregation, just analysis)
            sim::Criteria criteria;

            if (opt.patterns_month == 0) {
                auto year_data = sim.generate_year(sim::SimulatorProfile::Weekday);
                auto ys = sim::analyse_year(year_data, criteria);
                sim::print_year_summary(ys);
            } else {
                auto month_data = sim.generate_month(sim::SimulatorProfile::Weekday,
                                                     opt.patterns_month);
                auto ms = sim::analyse_month(month_data, criteria);
                sim::print_month_summary(ms);
            }
        } else {
            // Normal streaming simulation into Aggregator
            sim.start(sim::SimulatorProfile::Weekday);

            // 4. Compute end time using --sim-hours
            system_clock::time_point end = start + hours(opt.sim_hours);

            // 5. Simulation loop
            while (clock.now() < end) {
                auto batch = sim.next_batch(static_cast<int>(opt.batch_size));
                for (auto& rec : batch) {
                    accept_record(rec);
                }
            }
        }
    }

    // Final summary for now (simple console output)
    auto sum = agg.summary();
    std::cout << "Total ingested rows: " << sum.total_count << "\n";
    std::cout << "Per zone:\n";
    for (auto& [zone, count] : sum.by_zone) {
        std::cout << "  zone " << zone << ": " << count << "\n";
    }

    return 0;
}
