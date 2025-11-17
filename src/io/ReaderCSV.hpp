#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include "../model/SensorRecord.hpp"

namespace io {

class ReaderCSV {
public:
    explicit ReaderCSV(std::vector<std::string> inputs)
      : inputs_(std::move(inputs)), current_file_index_(0) {
        if (inputs_.empty()) {
            throw std::invalid_argument("No input files provided");
        }
        open_next_file();
    }

    std::vector<model::SensorRecord> next_batch(std::size_t max_rows = 1000);

    // Statistics for diagnostics
    struct ParseStats {
        size_t total_rows = 0;
        size_t parsed_rows = 0;
        size_t malformed_rows = 0;
        std::vector<std::string> errors;
    };

    const ParseStats& get_stats() const { return stats_; }

private:
    std::vector<std::string> inputs_;
    size_t current_file_index_;
    std::ifstream current_stream_;
    std::map<std::string, size_t> column_map_;
    bool header_parsed_ = false;
    ParseStats stats_;

    void open_next_file();
    void parse_header(const std::string& line);
    std::vector<std::string> split_line(const std::string& line) const;
    std::optional<model::SensorRecord> parse_row(const std::vector<std::string>& fields);
    std::chrono::system_clock::time_point parse_timestamp(const std::string& ts_str);
    std::string to_lower(const std::string& str) const;
    std::optional<double> parse_double(const std::string& str) const;
    std::optional<int> parse_int(const std::string& str) const;
};

} // namespace io
