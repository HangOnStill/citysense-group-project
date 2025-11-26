#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <algorithm>
#include <cctype>

#include "../model/SensorRecord.hpp"

namespace io {

    /**
     * ReaderJSON
     *
     * Very simple JSON reader for newline-delimited JSON records (NDJSON).
     *
     * Expected format for each non-empty line:
     *
     *   {
     *     "ts": 1700000000,
     *     "sensor_id": "air-1",
     *     "zone_id": 1,
     *     "pm25": 12.3,
     *     "pm10": 25.4,
     *     "db": 65.0,
     *     "speed": 42.1,
     *     "flow": 120.0
     *   }
     *
     * Notes:
     *  - "ts" is an integer Unix timestamp (seconds since epoch, UTC).
     *  - "sensor_id" is a string.
     *  - "zone_id" is an integer.
     *  - Other numeric fields are optional; if missing, the corresponding
     *    SensorRecord optional stays disengaged.
     *
     * Malformed lines are skipped instead of crashing, similar to ReaderCSV.
     */
    class ReaderJSON {
    public:
        explicit ReaderJSON(std::vector<std::string> inputs)
            : inputs_(std::move(inputs)) {
        }

        ReaderJSON(std::initializer_list<std::string> inputs)
            : inputs_(inputs) {
        }

        // Streaming-style: returns up to max_rows records across one or more files.
        // Keeps state between calls; when all inputs are exhausted, returns {}.
        std::vector<model::SensorRecord> next_batch(std::size_t max_rows) {
            std::vector<model::SensorRecord> out;
            out.reserve(max_rows);

            while (out.size() < max_rows) {
                if (!stream_.is_open()) {
                    if (current_file_index_ >= inputs_.size()) {
                        // No more files to read.
                        break;
                    }
                    open_next_file();
                }

                std::string line;
                while (out.size() < max_rows && std::getline(stream_, line)) {
                    trim_in_place(line);
                    if (line.empty()) {
                        continue;
                    }

                    try {
                        auto rec = parse_line(line);
                        out.push_back(std::move(rec));
                    }
                    catch (const std::exception&) {
                        // Skip malformed JSON lines; do not crash.
                        continue;
                    }
                }

                if (!stream_.good()) {
                    // EOF or error: close and move to next file.
                    stream_.close();
                    ++current_file_index_;
                }
            }

            return out;
        }

    private:
        // --- Small helpers ------------------------------------------------------

        static void trim_in_place(std::string& s) {
            auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
            auto it1 = std::find_if(s.begin(), s.end(), not_space);
            auto it2 = std::find_if(s.rbegin(), s.rend(), not_space).base();
            if (it1 == s.end()) {
                s.clear();
                return;
            }
            s.assign(it1, it2);
        }

        static std::optional<std::string> extract_string(const std::string& line,
            std::string_view key) {
            const std::string pattern = "\"" + std::string(key) + "\"";
            auto pos = line.find(pattern);
            if (pos == std::string::npos) return std::nullopt;

            pos = line.find(':', pos + pattern.size());
            if (pos == std::string::npos) return std::nullopt;

            ++pos; // move past ':'
            while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
                ++pos;
            }
            if (pos >= line.size() || line[pos] != '"') return std::nullopt;

            ++pos; // skip opening quote
            auto end = line.find('"', pos);
            if (end == std::string::npos) return std::nullopt;

            return line.substr(pos, end - pos);
        }

        static std::optional<std::int64_t> extract_int64(const std::string& line,
            std::string_view key) {
            const std::string pattern = "\"" + std::string(key) + "\"";
            auto pos = line.find(pattern);
            if (pos == std::string::npos) return std::nullopt;

            pos = line.find(':', pos + pattern.size());
            if (pos == std::string::npos) return std::nullopt;

            ++pos; // past ':'
            while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
                ++pos;
            }
            if (pos >= line.size()) return std::nullopt;

            auto start = pos;
            while (pos < line.size() &&
                (std::isdigit(static_cast<unsigned char>(line[pos])) ||
                    line[pos] == '-' || line[pos] == '+')) {
                ++pos;
            }
            if (start == pos) return std::nullopt;

            try {
                auto value = std::stoll(line.substr(start, pos - start));
                return value;
            }
            catch (...) {
                return std::nullopt;
            }
        }

        static std::optional<double> extract_double(const std::string& line,
            std::string_view key) {
            const std::string pattern = "\"" + std::string(key) + "\"";
            auto pos = line.find(pattern);
            if (pos == std::string::npos) return std::nullopt;

            pos = line.find(':', pos + pattern.size());
            if (pos == std::string::npos) return std::nullopt;

            ++pos; // past ':'
            while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
                ++pos;
            }
            if (pos >= line.size()) return std::nullopt;

            auto start = pos;
            while (pos < line.size() &&
                (std::isdigit(static_cast<unsigned char>(line[pos])) ||
                    line[pos] == '-' || line[pos] == '+' ||
                    line[pos] == '.' || line[pos] == 'e' || line[pos] == 'E')) {
                ++pos;
            }
            if (start == pos) return std::nullopt;

            try {
                auto value = std::stod(line.substr(start, pos - start));
                return value;
            }
            catch (...) {
                return std::nullopt;
            }
        }

        static std::chrono::system_clock::time_point
            epoch_seconds_to_timepoint(std::int64_t ts) {
            using namespace std::chrono;
            return system_clock::time_point{ seconds{ts} };
        }

        static model::SensorRecord parse_line(const std::string& line) {
            model::SensorRecord rec{};

            auto ts = extract_int64(line, "ts");
            auto sensor_id = extract_string(line, "sensor_id");
            auto zone_id = extract_int64(line, "zone_id");

            if (!ts || !sensor_id || !zone_id) {
                throw std::runtime_error("Missing mandatory JSON fields");
            }

            rec.ts = epoch_seconds_to_timepoint(*ts);
            rec.sensor_id = std::move(*sensor_id);
            rec.zone_id = static_cast<int>(*zone_id);

            if (auto v = extract_double(line, "pm25"))  rec.pm25 = *v;
            if (auto v = extract_double(line, "pm10"))  rec.pm10 = *v;
            if (auto v = extract_double(line, "db"))    rec.db = *v;
            if (auto v = extract_double(line, "speed")) rec.speed = *v;
            if (auto v = extract_double(line, "flow"))  rec.flow = *v;

            return rec;
        }

        void open_next_file() {
            const auto& path = inputs_.at(current_file_index_);
            stream_.close();
            stream_.clear();
            stream_.open(path);
            if (!stream_.is_open()) {
                throw std::runtime_error("Failed to open JSON input: " + path);
            }
        }

    private:
        std::vector<std::string> inputs_;
        std::size_t current_file_index_{ 0 };
        std::ifstream stream_;
    };

} // namespace io 
