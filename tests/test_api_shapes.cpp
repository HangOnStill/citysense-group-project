#include <catch2/catch_test_macros.hpp>
#include "core/Aggregator.hpp"
#include "core/Detector.hpp"
#include "core/Window.hpp"
#include "model/SensorRecord.hpp"
#include "io/ReaderCSV.hpp"

struct DummyDetector : core::Detector {
    void detect(const core::Window&) override {}
};

TEST_CASE("Public API surfaces exist (types, methods)") {
    core::Aggregator agg(5);
    core::Window w;
    DummyDetector d;
    d.detect(w);

    // Minimal consume/summary exercise
    agg.consume(std::vector<int>{1,2,3});
    auto s = agg.summary();
    REQUIRE(s.total_count >= 1);

    io::ReaderCSV reader({ "data/air.csv" });
    auto batch = reader.next_batch(10);
    REQUIRE(batch.size() >= 0);
}
