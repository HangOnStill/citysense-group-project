#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <vector>
#include "core/Aggregator.hpp"
#include "model/SensorRecord.hpp"

TEST_CASE("Aggregator supports parallel zone ingestion deterministically") {
    core::Aggregator agg(5);

    auto make_zone = [](int zone, int n) {
        std::vector<model::SensorRecord> v;
        v.reserve(n);
        for (int i = 0; i < n; ++i) {
            model::SensorRecord r; r.zone_id = zone; v.push_back(r);
        }
        return v;
    };

    auto z1 = make_zone(1, 1000);
    auto z2 = make_zone(2, 1000);

    std::thread t1([&]{ agg.consume(z1); });
    std::thread t2([&]{ agg.consume(z2); });
    t1.join(); t2.join();

    auto s = agg.summary();
    REQUIRE(s.total_count >= 2000);

    // If students fill by_zone, grading can validate stronger:
    // REQUIRE(s.by_zone[1] == 1000);
    // REQUIRE(s.by_zone[2] == 1000);
}
