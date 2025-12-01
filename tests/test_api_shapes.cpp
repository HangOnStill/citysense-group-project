#include <catch2/catch_test_macros.hpp>

#include "core/Aggregator.hpp"
#include "core/Window.hpp"
#include "core/Finding.hpp"
#include "model/SensorRecord.hpp"
#include "io/ReaderCSV.hpp"

struct DummyDetector : core::Detector {
    std::vector<core::Finding> detect(const core::Window&) override {
        return {};
    }
};

TEST_CASE("Public API surfaces exist (types, methods)") {
    core::Aggregator agg(5);
    core::Window w;
    DummyDetector d;
    auto findings = d.detect(w);
    (void)findings;

    // Minimal consume/summary exercise
    // use a dummy SensorRecord vector (works with new Aggregator)
    std::vector<model::SensorRecord> dummy(3);
    agg.consume(dummy);



    auto s = agg.summary();
    REQUIRE(s.total_count >= 1);
}
