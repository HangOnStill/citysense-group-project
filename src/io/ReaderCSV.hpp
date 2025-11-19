#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <string_view>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <optional>

#include "../model/SensorRecord.hpp"

namespace io {

    class ReaderCSV {
    public:
        explicit ReaderCSV(std::vector<std::string> inputs)
            : inputs_(std::move(inputs)) {
        }

        // Optional: constructor with a reserve hint
        ReaderCSV(std::vector<std::string> inputs, std::size_t reserve_hint)
            : inputs_(std::move(inputs)), reserve_hint_(reserve_hint) {
        }

        // Optional: setter for reserve hint
        void set_reserve_hint(std::size_t n) { reserve_hint_ = n; }

        std::vector<model::SensorRecord> next_batch(std::size_t max_rows = 1000) {
            if (max_rows == 0) return {};

            ensure_stream();  // may throw if no files can be opened at all

            std::vector<model::SensorRecord> batch;
            std::size_t to_reserve = max_rows;
            if (reserve_hint_ != 0) {
                to_reserve = std::min(to_reserve, reserve_hint_);
            }
            batch.reserve(to_reserve);

            std::string line;

            while (batch.size() < max_rows && has_stream_) {
                if (!std::getline(current_, line)) {
                    current_.close();
                    has_stream_ = false;
                    header_parsed_ = false;
                    ++current_index_;
                    if (!advance_to_next_stream()) {
                        break;
                    }
                    continue;
                }

                if (line.empty()) {
                    continue;
                }

                auto cells = split_line(line);
                if (cells.empty()) {
                    continue;
                }

                model::SensorRecord rec{};
                fill_record(cells, rec);
                batch.push_back(std::move(rec));
            }

            return batch;
        }

    private:
        std::vector<std::string> inputs_;
        std::size_t current_index_{ 0 };
        std::ifstream current_;
        bool has_stream_{ false };
        bool header_parsed_{ false };
        bool any_file_opened_{ false };
        std::size_t reserve_hint_{ 0 };

        struct Columns {
            int ts = -1;
            int sensor_id = -1;
            int zone_id = -1;
            int speed = -1;
            int flow = -1;
            int pm25 = -1;
            int pm10 = -1;
            int db = -1;
        };
        Columns cols_{};

        void ensure_stream() {
            if (!has_stream_) {
                if (!advance_to_next_stream()) {
                    if (!any_file_opened_) {
                        throw std::runtime_error("ReaderCSV: no input files could be opened");
                    }
                }
            }
        }

        bool advance_to_next_stream() {
            std::string header_line;

            while (current_index_ < inputs_.size()) {
                current_.close();
                current_.clear();

                current_.open(inputs_[current_index_]);
                if (!current_.is_open()) {
                    ++current_index_;
                    continue;
                }

                any_file_opened_ = true;

                if (!std::getline(current_, header_line)) {
                    ++current_index_;
                    continue;
                }

                parse_header(header_line);
                has_stream_ = true;
                header_parsed_ = true;
                return true;
            }

            has_stream_ = false;
            return false;
        }

        static std::vector<std::string> split_line(const std::string& line) {
            std::vector<std::string> cells;
            std::string cell;
            std::stringstream ss(line);

            while (std::getline(ss, cell, ',')) {
                auto l = cell.find_first_not_of(" \t\r\n");
                auto r = cell.find_last_not_of(" \t\r\n");
                if (l == std::string::npos) {
                    cells.emplace_back();
                }
                else {
                    cells.emplace_back(cell.substr(l, r - l + 1));
                }
            }
            return cells;
        }

        static std::string to_lower(const std::string& s) {
            std::string out;
            out.reserve(s.size());
            for (unsigned char ch : s) {
                out.push_back(static_cast<char>(std::tolower(ch)));
            }
            return out;
        }

        void parse_header(const std::string& header) {
            cols_ = Columns{};
            auto names = split_line(header);
            for (std::size_t i = 0; i < names.size(); ++i) {
                const auto name_lc = to_lower(names[i]);
                if (name_lc == "timestamp") {
                    cols_.ts = static_cast<int>(i);
                }
                else if (name_lc == "sensor_id") {
                    cols_.sensor_id = static_cast<int>(i);
                }
                else if (name_lc == "zone_id") {
                    cols_.zone_id = static_cast<int>(i);
                }
                else if (name_lc == "speed" || name_lc == "speed_kmh") {
                    cols_.speed = static_cast<int>(i);
                }
                else if (name_lc == "flow" || name_lc == "flow_per_min") {
                    cols_.flow = static_cast<int>(i);
                }
                else if (name_lc == "pm25") {
                    cols_.pm25 = static_cast<int>(i);
                }
                else if (name_lc == "pm10") {
                    cols_.pm10 = static_cast<int>(i);
                }
                else if (name_lc == "db" || name_lc == "dba") {
                    cols_.db = static_cast<int>(i);
                }
            }
        }

        static bool in_bounds(int idx, const std::vector<std::string>& cells) {
            return idx >= 0 && static_cast<std::size_t>(idx) < cells.size();
        }

        static std::optional<double> parse_double_safe(const std::vector<std::string>& cells, int idx) {
            if (!in_bounds(idx, cells)) return std::nullopt;
            const auto& s = cells[static_cast<std::size_t>(idx)];
            if (s.empty()) return std::nullopt;
            try {
                return std::stod(s);
            }
            catch (...) {
                return std::nullopt;
            }
        }

        void fill_record(const std::vector<std::string>& cells, model::SensorRecord& r) const {
            if (in_bounds(cols_.sensor_id, cells)) {
                r.sensor_id = cells[static_cast<std::size_t>(cols_.sensor_id)];
            }

            // For now, keep zone_id as simple int; later we can map zones.
            r.zone_id = 0;

            r.speed = parse_double_safe(cells, cols_.speed);
            r.flow = parse_double_safe(cells, cols_.flow);
            r.pm25 = parse_double_safe(cells, cols_.pm25);
            r.pm10 = parse_double_safe(cells, cols_.pm10);
            r.db = parse_double_safe(cells, cols_.db);
        }
    };

} // namespace io
