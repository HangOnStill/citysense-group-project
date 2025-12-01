// tests/test_parallel_ingest.cpp
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <vector>
#include <string>

#include "io/ReaderCSV.hpp"
#include "parallel/ZoneWorkers.hpp"
#include "core/Aggregator.hpp"
#include "model/SensorRecord.hpp"

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
    core::Aggregator serialAgg{ 0 };
    for (auto& r : records) serialAgg.consume(r);
    auto serialSummary = serialAgg.summary();

    // Zones present in the data
    std::vector<int> zones;
    zones.reserve(records.size());
    for (auto& r : records) zones.push_back(r.zone_id);
    std::sort(zones.begin(), zones.end());
    zones.erase(std::unique(zones.begin(), zones.end()), zones.end());

    // Parallel per zone
    parallel::ZoneIngestor par{ zones };
    par.start_workers();
    for (auto& r : records) par.ingest(r);
    auto perZoneSummaries = par.finish();

    // Recombine per-zone summaries
    core::Summary parallelSummary;
    parallelSummary.total_count = 0;
    for (auto& z : perZoneSummaries) {
        parallelSummary.total_count += z.summary.total_count;
        for (auto& [zone, count] : z.summary.by_zone) {
            parallelSummary.by_zone[zone] += count;
        }
    }

    REQUIRE(parallelSummary.total_count == serialSummary.total_count);
    REQUIRE(parallelSummary.by_zone == serialSummary.by_zone);
}
