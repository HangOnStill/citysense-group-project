// tests/test_parallel_ingest.cpp
#include <catch2/catch_test_macros.hpp>
#include "io/ReaderCSV.hpp"
#include "parallel/ZoneWorkers.hpp"
#include "agg/Aggregator.hpp"

TEST_CASE("parallel ingestion matches serial") {
    std::vector<std::string> files = { "data/sample.csv" };
    io::ReaderCSV reader{ files };

    // Load everything once
    std::vector<model::SensorRecord> records;
    while (true) {
        auto batch = reader.next_batch(500);
        if (batch.empty()) break;
        records.insert(records.end(),
            std::make_move_iterator(batch.begin()),
            std::make_move_iterator(batch.end()));
    }

    // Serial
    agg::Aggregator serialAgg;
    for (auto& r : records) serialAgg.consume(r);
    auto serialSummary = serialAgg.finalize();

    // Parallel per zone
    std::vector<int> zones; // fill from data if you like; or hardcode
    for (auto& r : records) zones.push_back(r.zone_id);
    std::sort(zones.begin(), zones.end());
    zones.erase(std::unique(zones.begin(), zones.end()), zones.end());

    parallel::ZoneIngestor par{ zones };
    par.start_workers();
    for (auto& r : records) par.ingest(r);
    auto perZoneSummaries = par.finish();

    // Recombine per-zone summaries into a "combined" one however makes sense
    // For now assume Aggregator has merge(Summary).
    agg::Aggregator merged;
    for (auto& z : perZoneSummaries) {
        merged.merge(z.summary);
    }
    auto parallelSummary = merged.finalize();

    REQUIRE(parallelSummary == serialSummary); // define == or compare key fields
}
