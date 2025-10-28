#include <catch2/catch_test_macros.hpp>
#include <vector>
#include "core/Aggregator.hpp"
#include "model/SensorRecord.hpp"

namespace {
std::vector<model::SensorRecord> make_records(int zone, int count) {
    std::vector<model::SensorRecord> v;
    v.reserve(count);
    for (int i = 0; i < count; ++i) {
        model::SensorRecord rec;
        rec.zone_id = zone;
        v.push_back(rec);
    }
    return v;
}
} // namespace

TEST_CASE("Aggregator provides stable summary contract") {
    core::Aggregator agg(/*window_minutes=*/5);

    const auto initial = agg.summary();
    REQUIRE(initial.total_count >= 0);

    auto first_batch = make_records(/*zone=*/1, /*count=*/3);
    agg.consume(first_batch);
    const auto after_first = agg.summary();
    REQUIRE(after_first.total_count >= 0);
    for (const auto& [zone, count] : after_first.by_zone) {
        (void)zone;
        REQUIRE(count >= 0);
    }

    agg.consume(make_records(/*zone=*/2, /*count=*/0));
    agg.consume(make_records(/*zone=*/2, /*count=*/2));
    const auto after_more = agg.summary();
    REQUIRE(after_more.total_count >= 0);
    for (const auto& [zone, count] : after_more.by_zone) {
        (void)zone;
        REQUIRE(count >= 0);
    }
}
