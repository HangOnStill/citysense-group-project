#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>
#include <optional>
#include "io/ReaderCSV.hpp"
#include "model/SensorRecord.hpp"

static bool has_air_fields(const model::SensorRecord& r) {
    return r.pm25.has_value() || r.pm10.has_value();
}

TEST_CASE("CSV reader parses header case-insensitively and returns records") {
    io::ReaderCSV r({ "data/air.csv" });
    auto batch = r.next_batch(100);
    REQUIRE(batch.size() >= 1);                // must read at least one row
    REQUIRE(has_air_fields(batch.front()));    // pm fields should map into optionals
}

TEST_CASE("CSV reader supports required columns per family") {
    // air: timestamp,sensor_id,zone_id,pm25,pm10
    io::ReaderCSV air({ "data/air.csv" });
    auto a = air.next_batch(10);
    REQUIRE(a.size() >= 1);

    // noise: timestamp,sensor_id,zone_id,db
    io::ReaderCSV noise({ "data/noise.csv" });
    auto n = noise.next_batch(10);
    REQUIRE(n.size() >= 1);
    REQUIRE(n.front().db.has_value());

    // traffic: timestamp,sensor_id,zone_id,speed,flow
    io::ReaderCSV traffic({ "data/traffic.csv" });
    auto t = traffic.next_batch(10);
    REQUIRE(t.size() >= 1);
    REQUIRE(t.front().speed.has_value());
    REQUIRE(t.front().flow.has_value());
}
