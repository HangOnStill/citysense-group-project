#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include "io/ReaderCSV.hpp"

TEST_CASE("ReaderCSV throws on missing file") {
    io::ReaderCSV r({ "data/DOES_NOT_EXIST.csv" });
    // Contract: next_batch should throw std::runtime_error if no inputs can be opened.
    REQUIRE_THROWS_AS(r.next_batch(1), std::runtime_error);
}
