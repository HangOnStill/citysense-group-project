#pragma once
#include <string>
#include <vector>
#include "../model/SensorRecord.hpp"

namespace io {

// Minimal contract only. Students must implement real parsing.
class ReaderCSV {
public:
    explicit ReaderCSV(std::vector<std::string> inputs)
      : inputs_(std::move(inputs)) {}

    std::vector<model::SensorRecord> next_batch(std::size_t /*max_rows*/ = 1000) {
        // TODO: parse CSV headers, map to SensorRecord fields, return batch
        return {};
    }

private:
    std::vector<std::string> inputs_;
};

} // namespace io
