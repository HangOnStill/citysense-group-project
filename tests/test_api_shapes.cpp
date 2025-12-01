// src/tests/test_api_shapes.cpp
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "core/Aggregator.hpp"
#include "core/Window.hpp"
#include "core/Detector.hpp"
#include "core/Finding.hpp"
#include "model/SensorRecord.hpp"
#include "io/ReaderCSV.hpp"

struct DummyDetector : core::Detector {
    std::vector<core::Finding> detect(const core::Window&) override {
        // no-op detector: returns an empty finding list
        return {};
    }
};

TEST_CASE("Public API surfaces exist (types, methods)") {
    core::Aggregator agg(5);
    core::Window w;
    DummyDetector d;

    // Ensure the method exists and is callable
    auto findings = d.detect(w);
    (void)findings;

    // Minimal consume/summary exercise using SensorRecord,
    // matching the Aggregator::consume Range-style contract.
    std::vector<model::SensorRecord> recs(3); // default-initialized records
    agg.consume(recs);
    auto s = agg.summary();
    REQUIRE(s.total_count >= 1);

    // Ensure ReaderCSV is constructible and can read something from air.csv
    io::ReaderCSV reader({ "data/air.csv" });
    auto batch = reader.next_batch(10);
    REQUIRE(batch.size() >= 1);
}
