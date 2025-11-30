#include <iostream>
#include "app/Options.hpp"
#include "app/Options.cpp"
#include "io/ReaderCSV.hpp"
#include "sim/Simulator.hpp"
#include "sim/Simulator.cpp"
#include "sim/PatternAnalytics.hpp"
#include "sim/PatternAnalytics.cpp"
#include "sim/Criteria.hpp"
#include "sim/SeededRNG.hpp"
#include "sim/Clock.hpp"


#include "core/Aggregator.hpp"      
#include "core/Window.hpp"          
#include "core/Finding.hpp"         

#include "detectors/AirAlert.hpp"         
#include "detectors/NoiseSpike.hpp"        
#include "detectors/TrafficCongestion.hpp"  



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
    core::Aggregator agg{ 10 };
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
    system_clock::time_point start;
    if (opt.from) {
        start = *opt.from;
    } else {
        start = system_clock::now();
    }
    int step_seconds = 60;
    sim::Clock clock{ start, step_seconds };

    sim::SeededRNG rng{ opt.sim_seed };
    sim::Simulator sim{ clock, rng };
    sim::Criteria criteria;
    if (opt.run_patterns){
        if (opt.patterns_month == 0){
            auto year_data = sim.generate_year(sim::SimulatorProfile::Weekday);
            auto year_data_aggregated = sim::analyse_year(year_data, criteria);
            sim::print_year_summary(year_data_aggregated);
        }else{
            auto month_data = sim.generate_month(sim::SimulatorProfile::Weekday, opt.patterns_month);
            auto month_data_aggregated = sim::analyse_month(month_data, criteria);
            sim::print_month_summary(month_data_aggregated);
        }
    }else{
        sim.start(sim::SimulatorProfile::Weekday);

        system_clock::time_point end = start + hours(opt.sim_hours);

        detectors::AirAlert air_detector{ 35.0 };
        detectors::NoiseSpike noise_detector{ 85.0, 10, 10 };
        detectors::TrafficCongestion traffic_detector{ 25.0, 3 };

        std::vector<core::Detector*> dets = {
            &air_detector,
            &noise_detector,
            &traffic_detector
        };

        while (clock.now() < end) {
            auto batch = sim.next_batch(static_cast<int>(opt.batch_size));

            for (auto& rec : batch)
                accept_record(rec);

            const core::Window& win = agg.current_window_view();

            for (auto* det : dets) {
                auto findings = det->detect(win);
                for (const auto& f : findings) {
                    std::cout << "[" << f.detector << "] "
                            << "value=" << f.value
                            << "  window=" << f.start_ts.time_since_epoch().count()
                            << " → "       << f.end_ts.time_since_epoch().count()
                            << "\n";
                }
            }
}       
    }
    
}
    auto sum = agg.summary();
    (void)sum;

    return 0;
}
