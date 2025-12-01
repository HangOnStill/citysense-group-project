// tests/test_api_shapes.cpp
#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <string>

#include "core/Aggregator.hpp"
#include "core/Window.hpp"
#include "core/Detector.hpp"
#include "core/Finding.hpp"
#include "model/SensorRecord.hpp"
#include "io/ReaderCSV.hpp"

// Simple no-op detector implementation to exercise the Detector API.
struct DummyDetector : core::Detector {
    std::vector<core::Finding> detect(const core::Window&) override {
        return {};
    }
};

TEST_CASE("Public API surfaces exist (types, methods)") {
    // Aggregator should be constructible with a window size (e.g. 5 minutes)
    core::Aggregator agg{ 5 };

    // Window type must be default-constructible
    core::Window w;

    // Detector base must be subclassable and callable
    DummyDetector d;
    auto findings = d.detect(w);
    (void)findings; // silence unused warning

    // Minimal consume/summary exercise using SensorRecord,
    // matching the Aggregator::consume Range-style contract.
    std::vector<model::SensorRecord> recs(3); // default-initialized records
    agg.consume(recs);
    auto s = agg.summary();
    REQUIRE(s.total_count >= 1);

    // Ensure ReaderCSV is constructible and can read something from air.csv
    // TEST_DATA_DIR is defined for this target in CMakeLists.txt.
    #ifdef TEST_DATA_DIR
        std::string air_path = std::string(TEST_DATA_DIR) + "/air.csv";
    #else
        std::string air_path = "data/air.csv"; // fallback, should not happen in CI
    #endif

        io::ReaderCSV reader({ air_path });
        auto batch = reader.next_batch(10);

        // We only care that it does *something* and does not throw.
        REQUIRE(batch.size() >= 1);

}

