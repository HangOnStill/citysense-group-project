#include <iostream>
#include <chrono>
#include <cassert>

#include "sim/Simulator.hpp"
#include "sim/SimulatorProfile.hpp"
#include "sim/PatternAnalytics.hpp"
#include "sim/Criteria.hpp"


using namespace std::chrono;

int main(){
system_clock::time_point start{seconds{0}};
    sim::Clock clock{start, 60};
    sim::SeededRNG rng{1234};
    sim::Simulator sim{clock, rng};
    auto profile = sim::SimulatorProfile::Weekday;
    sim::Criteria criteria;

    auto year_data = sim.generate_year(profile);
    auto year_data_aggregated = sim::analyse_year(year_data, criteria);
    sim::print_year_summary(year_data_aggregated);

    auto month_data = sim.generate_month(profile, 1);
    auto month_data_aggregated = sim::analyse_month(month_data, criteria);
    sim::print_month_summary(month_data_aggregated);

}

/*
void print_record(const model::SensorRecord& rec)

{

    

    
    if (rec.sensor_id == "traffic-glebe" || rec.sensor_id == "traffic-downtown" || rec.sensor_id == "traffic-byward"){
        std::cout << rec.ts << " - "  << rec.sensor_id << " "
              << (rec.speed.has_value() ? std::to_string(*rec.speed) : "-")
              << "\n";
    }else if(rec.sensor_id == "air-glebe" || rec.sensor_id == "air-downtown" || rec.sensor_id == "air-byward"){
        std::cout << rec.ts << " - "  << rec.sensor_id << " "
              << (rec.pm25.has_value() ? std::to_string(*rec.pm25) : "-")
              << "\n";
    }else{
        std::cout << rec.ts << " - " << rec.sensor_id << " "
              << (rec.db.has_value() ? std::to_string(*rec.db) : " - ")
              << "\n";
    }
    
}

int main(){

    // --- create base simulator ---
    system_clock::time_point start{seconds{0}};
    sim::Clock clock{start, 60};
    sim::SeededRNG rng{1234};
    sim::Simulator sim{clock, rng};
    auto profile = sim::SimulatorProfile::Weekday;
    //sim.start(profile);
    
    auto step1 = sim.next_step();
    for (auto step : step1){
        print_record(step);
    }
    std::cout << "\n";
    auto step2 = sim.next_step();
    for (auto step : step2){
        print_record(step);
    }

    std::cout << "\n\n";
    system_clock::time_point start2{seconds{0}};
    sim::Clock clock2{start2, 60};
    sim::SeededRNG rng2{12345};
    sim::Simulator sim2{clock2, rng2};

    sim2.start(sim::SimulatorProfile::Weekday);

    auto step3 = sim2.next_step();
    for (auto step : step3){
        print_record(step);
    }
    std::cout << "\n";
    auto step4 = sim2.next_step();
    for (auto step : step4){
        print_record(step);
    }
    

    auto year = sim.generate_year(profile);
    for (auto step : year){
        print_record(step);
    }
    /*
    // ============================================================
    // 1. Test start() and next_step()
    // ============================================================

    sim.start(sim::SimulatorProfile::Weekday);

    auto step1 = sim.next_step();
    auto step2 = sim.next_step();

    assert(step1.size() == 9);

    assert(step2.size() == 9);

    std::cout << "next_step(): OK (produced 9 records per step)\n";

    // ============================================================
    // 2. Test determinism — rebuilding the simulator should produce identical steps
    // ============================================================

    sim::Clock clock2{start, 60};
    sim::SeededRNG rng2{1234};
    sim::Simulator sim2{clock2, rng2};

    sim2.start(sim::SimulatorProfile::Weekday);

    auto step1_b = sim2.next_step();
    auto step2_b = sim2.next_step();

    // compare the first field of first record as a sanity check
    assert(step1_b[0].sensor_id == step1[0].sensor_id);
    assert(step2_b[0].sensor_id == step2[0].sensor_id);
    assert(step1_b[0].speed.value() == step1[0].speed.value());

    std::cout << "Determinism test: OK (next_step output matches with same seed)\n";

    // ============================================================
    // 3. Test next_batch()
    // ============================================================

    sim2.start(sim::SimulatorProfile::Weekday); // reset
    auto batch = sim2.next_batch(5); // 5 minutes

    assert(batch.size() == 5 * 9);

    std::cout << "next_batch(): OK (45 records for 5 steps)\n";

    // ============================================================
    // 4. Test generate_day() produces same results as 1440 next_step()
    // ============================================================

    sim::Clock c_day{start, 60};
    sim::SeededRNG r_day{1234};
    sim::Simulator sim_day{c_day, r_day};

    auto full_day_time = sim_day.generate_day(sim::SimulatorProfile::Weekday);

    // Recreate using next_step
    sim::Clock c_step{start, 60};
    sim::SeededRNG r_step{1234};
    sim::Simulator sim_step{c_step, r_step};
    sim_step.start(sim::SimulatorProfile::Weekday);

    std::vector<model::SensorRecord> from_steps;
    from_steps.reserve(1440 * 9);

    for (int i = 0; i < 1440; i++)
    {
        auto step = sim_step.next_step();
        from_steps.insert(from_steps.end(), step.begin(), step.end());
    }

    // Compare a few sample points
    assert(from_steps[0].speed == full_day_time[0].speed);
    assert(from_steps[200].pm25 == full_day_time[200].pm25);
    assert(from_steps[1000].db == full_day_time[1000].db);

    std::cout << "generate_day() matches 1440×next_step(): OK\n";

    // ============================================================
    // 5. Optional: check that Weekend profile differs from Weekday
    // ============================================================

    sim::Clock wc{start, 60};
    sim::SeededRNG wr{1234};
    sim::Simulator weekend{wc, wr};

    weekend.start(sim::SimulatorProfile::Weekend);
    auto wstep = weekend.next_step();

    assert(wstep[0].speed != step1[0].speed);

    std::cout << "Weekend vs Weekday differ: OK\n";

    std::cout << "=== ALL TESTS PASSED ===\n";
    
}
    */
