#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <chrono>

#include "../model/SensorRecord.hpp"

namespace io {

    // Streaming CSV reader over one or more input files.
    class ReaderCSV {
    public:
        explicit ReaderCSV(std::vector<std::string> inputs)
            : inputs_(std::move(inputs)) {
        }

        // Returns up to max_rows SensorRecords.
        // Contract: if no inputs can be opened at all, throws std::runtime_error.
        std::vector<model::SensorRecord> next_batch(std::size_t max_rows = 1000) {
            std::vector<model::SensorRecord> batch;
            if (max_rows == 0) {
                return batch;
            }

            for (;;) {
                // Ensure we have an open data stream; may throw if *no* file
                // can ever be opened (as per contract).
                if (!ensure_stream()) {
                    // All files exhausted; return what we have (possibly empty).
                    return batch;
                }

                std::string line;
                while (batch.size() < max_rows && std::getline(current_, line)) {
                    if (line.empty()) {
                        continue;
                    }

                    auto cols = split(line);
                    if (cols.empty()) {
                        ++malformed_count_;
                        continue;
                    }

                    model::SensorRecord rec{};
                    // CSV does not carry a timestamp in this project;
                    // default-construct to a well-defined value.
                    rec.ts = std::chrono::system_clock::time_point{};

                    // sensor_id (string, case-insensitive header)
                    if (auto s = get_string(cols, "sensor_id")) {
                        rec.sensor_id = *s;
                    } else {
                        rec.sensor_id.clear();
                    }

                    // zone_id (stable small integer per zone string)
                    rec.zone_id = get_zone(cols, "zone_id");

                    // family-specific numeric fields (optional)
                    rec.speed = parse_double(cols, "speed");
                    rec.flow  = parse_double(cols, "flow");
                    rec.pm25  = parse_double(cols, "pm25");
                    rec.pm10  = parse_double(cols, "pm10");
                    rec.db    = parse_double(cols, "db");

                    batch.push_back(std::move(rec));
                }

                // If we produced any rows, or we are on the last file, return.
                if (!batch.empty() || current_index_ + 1 >= inputs_.size()) {
                    return batch;
                }

                // Otherwise, this file is exhausted and batch is still empty:
                // move to the next file and loop again.
                current_.close();
                ++current_index_;
            }
        }

    private:
        std::vector<std::string> inputs_;
        std::size_t current_index_{0};
        std::ifstream current_;
        bool any_file_opened_{false};
        

        // Lower-cased column name -> column index
        std::unordered_map<std::string, std::size_t> header_index_;

        // Zone string -> stable small int id
        std::unordered_map<std::string, int> zone_map_;
        int next_zone_id_{1};


        std::size_t malformed_count_{ 0 };
    public:
        std::size_t malformed_count() const noexcept { return malformed_count_; }

        // Open current file (or next) and read header.
        // Returns true if a stream is open and ready, false if exhausted.
        // Open current file (or next) and read header.
// Returns true if a stream is open and ready, false if exhausted.
        bool ensure_stream() {
            using std::string;

            while (!current_.is_open() && current_index_ < inputs_.size()) {
                current_.close();
                current_.clear();

                const string raw = inputs_[current_index_];

                // Extract just the filename part (naive, but enough here).
                string filename = raw;
                auto pos = raw.find_last_of("/\\");
                if (pos != string::npos) {
                    filename = raw.substr(pos + 1);
                }

                // Try a few reasonable candidate paths.
                std::vector<string> candidates;
                candidates.push_back(raw);                      // as passed
                candidates.push_back("../" + raw);              // from build/ up one
                candidates.push_back("data/" + filename);       // data/filename
                candidates.push_back("../data/" + filename);    // ../data/filename

                bool opened = false;
                for (const auto& path : candidates) {
                    current_.close();
                    current_.clear();
                    current_.open(path);
                    if (current_) {
                        any_file_opened_ = true;
                        opened = true;
                        break;
                    }
                }

                if (!opened) {
                    // None of the candidate paths worked; move to next input.
                    ++current_index_;
                    continue;
                }

                // We have an open stream now; read header.
                std::string header;
                if (!std::getline(current_, header)) {
                    // Empty file; skip it and try next input.
                    current_.close();
                    ++current_index_;
                    continue;
                }

                parse_header(header);
                return true;
            }

            if (!any_file_opened_ && !current_.is_open()) {
                // No file could be opened at all – contract says: throw.
                throw std::runtime_error("ReaderCSV: could not open any input file");
            }

            return current_.is_open();
        }


        void parse_header(const std::string& line) {
            header_index_.clear();
            auto cols = split(line);
            for (std::size_t i = 0; i < cols.size(); ++i) {
                auto name = trim(cols[i]);
                header_index_[to_lower(name)] = i;
            }
        }

        // Lowercase helper
        static std::string to_lower(std::string s) {
            std::transform(
                s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        // Trim leading + trailing whitespace and CR.
        static std::string trim(std::string s) {
            // leading
            std::size_t start = 0;
            while (start < s.size() &&
                   (s[start] == ' ' || s[start] == '\t' || s[start] == '\r')) {
                ++start;
            }
            // trailing
            std::size_t end = s.size();
            while (end > start &&
                   (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
                --end;
            }
            return s.substr(start, end - start);
        }

        // Very small CSV splitter (no quoting support; OK for provided data).
        std::vector<std::string> split(const std::string& line) const {
            std::vector<std::string> out;
            std::string cur;
            for (char ch : line) {
                if (ch == ',') {
                    out.push_back(trim(cur));
                    cur.clear();
                } else {
                    cur.push_back(ch);
                }
            }
            out.push_back(trim(cur));
            return out;
        }

        int zone_id_for(const std::string& zone) {
            auto it = zone_map_.find(zone);
            if (it != zone_map_.end()) {
                return it->second;
            }
            int id = next_zone_id_++;
            zone_map_[zone] = id;
            return id;
        }

        // Helper: fetch a string column by (case-insensitive) name.
        std::optional<std::string> get_string(
            const std::vector<std::string>& cols,
            const std::string& name) const
        {
            auto key = to_lower(name);
            auto it  = header_index_.find(key);
            if (it == header_index_.end()) return std::nullopt;
            std::size_t idx = it->second;
            if (idx >= cols.size()) return std::nullopt;

            auto val = trim(cols[idx]);
            if (val.empty()) return std::nullopt;
            return val;
        }

        // Helper: fetch / map zone_id; returns 0 if column missing or empty.
        int get_zone(const std::vector<std::string>& cols,
                     const std::string& name)
        {
            auto opt = get_string(cols, name);
            if (!opt) {
                return 0;
            }
            return zone_id_for(*opt);
        }

		// Helper: parse a double column by (case-insensitive) name.
        std::optional<double> parse_double(
            const std::vector<std::string>& cols,
            const std::string& name) const
        {
            auto key = to_lower(name);
            auto it  = header_index_.find(key);
            if (it == header_index_.end()) return std::nullopt;

            std::size_t idx = it->second;
            if (idx >= cols.size()) return std::nullopt;

            auto s = trim(cols[idx]);
            if (s.empty()) return std::nullopt;

            try {
                std::size_t pos = 0;
                double v = std::stod(s, &pos);
                if (pos == 0) {
                    // nothing parsed at all → malformed numeric field
                    return std::nullopt;
                }
                return v;
            }
            catch (...) {
                return std::nullopt;
            }
        }
    };

} // namespace io
