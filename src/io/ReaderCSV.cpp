#include "ReaderCSV.hpp"
#include <iostream>

namespace io {

void ReaderCSV::open_next_file() {
    if (current_stream_.is_open()) {
        current_stream_.close();
    }

    if (current_file_index_ >= inputs_.size()) {
        return; // No more files to process
    }

    const auto& filepath = inputs_[current_file_index_];
    current_stream_.open(filepath);
    
    if (!current_stream_.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    header_parsed_ = false;
    column_map_.clear();
}

std::vector<model::SensorRecord> ReaderCSV::next_batch(std::size_t max_rows) {
    std::vector<model::SensorRecord> records;
    records.reserve(max_rows);

    std::string line;
    size_t rows_read = 0;

    while (rows_read < max_rows) {
        // If current file is exhausted, try next file
        if (!std::getline(current_stream_, line)) {
            current_file_index_++;
            if (current_file_index_ >= inputs_.size()) {
                break; // No more files
            }
            open_next_file();
            continue;
        }

        // Skip empty lines
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }

        // Parse header on first data line
        if (!header_parsed_) {
            parse_header(line);
            header_parsed_ = true;
            continue;
        }

        // Parse data row
        stats_.total_rows++;
        auto fields = split_line(line);
        
        try {
            auto record = parse_row(fields);
            if (record.has_value()) {
                records.push_back(std::move(record.value()));
                stats_.parsed_rows++;
                rows_read++;
            } else {
                stats_.malformed_rows++;
            }
        } catch (const std::exception& e) {
            stats_.malformed_rows++;
            stats_.errors.push_back("Row " + std::to_string(stats_.total_rows) + 
                                   ": " + e.what());
            // Continue processing next row
        }
    }

    return records;
}

void ReaderCSV::parse_header(const std::string& line) {
    auto fields = split_line(line);
    
    for (size_t i = 0; i < fields.size(); ++i) {
        std::string normalized = to_lower(fields[i]);
        // Remove whitespace
        normalized.erase(std::remove_if(normalized.begin(), normalized.end(), ::isspace), 
                        normalized.end());
        column_map_[normalized] = i;
    }

    // Validate required columns
    if (column_map_.find("timestamp") == column_map_.end()) {
        throw std::runtime_error("Missing required column: timestamp");
    }
    if (column_map_.find("sensor_id") == column_map_.end() && 
        column_map_.find("sensorid") == column_map_.end()) {
        throw std::runtime_error("Missing required column: sensor_id");
    }
    if (column_map_.find("zone_id") == column_map_.end() && 
        column_map_.find("zoneid") == column_map_.end()) {
        throw std::runtime_error("Missing required column: zone_id");
    }
}

std::vector<std::string> ReaderCSV::split_line(const std::string& line) const {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;

    while (std::getline(ss, field, ',')) {
        // Trim whitespace
        size_t start = field.find_first_not_of(" \t\r\n");
        size_t end = field.find_last_not_of(" \t\r\n");
        
        if (start != std::string::npos && end != std::string::npos) {
            fields.push_back(field.substr(start, end - start + 1));
        } else {
            fields.push_back("");
        }
    }

    return fields;
}

std::optional<model::SensorRecord> ReaderCSV::parse_row(const std::vector<std::string>& fields) {
    if (fields.empty()) {
        return std::nullopt;
    }

    model::SensorRecord record;

    // Parse required fields
    try {
        // Timestamp
        auto ts_it = column_map_.find("timestamp");
        if (ts_it != column_map_.end() && ts_it->second < fields.size()) {
            record.ts = parse_timestamp(fields[ts_it->second]);
        } else {
            return std::nullopt; // Missing timestamp
        }

        // Sensor ID
        auto sensor_it = column_map_.find("sensor_id");
        if (sensor_it == column_map_.end()) {
            sensor_it = column_map_.find("sensorid");
        }
        if (sensor_it != column_map_.end() && sensor_it->second < fields.size()) {
            record.sensor_id = fields[sensor_it->second];
        } else {
            return std::nullopt; // Missing sensor_id
        }

        // Zone ID
        auto zone_it = column_map_.find("zone_id");
        if (zone_it == column_map_.end()) {
            zone_it = column_map_.find("zoneid");
        }
        if (zone_it != column_map_.end() && zone_it->second < fields.size()) {
            auto zone_opt = parse_int(fields[zone_it->second]);
            if (zone_opt.has_value()) {
                record.zone_id = zone_opt.value();
            } else {
                return std::nullopt; // Invalid zone_id
            }
        } else {
            return std::nullopt; // Missing zone_id
        }

        // Parse optional fields - Traffic
        auto speed_it = column_map_.find("speed");
        if (speed_it != column_map_.end() && speed_it->second < fields.size()) {
            record.speed = parse_double(fields[speed_it->second]);
        }

        auto flow_it = column_map_.find("flow");
        if (flow_it != column_map_.end() && flow_it->second < fields.size()) {
            record.flow = parse_double(fields[flow_it->second]);
        }

        // Parse optional fields - Air Quality
        auto pm25_it = column_map_.find("pm25");
        if (pm25_it == column_map_.end()) {
            pm25_it = column_map_.find("pm2.5");
        }
        if (pm25_it != column_map_.end() && pm25_it->second < fields.size()) {
            record.pm25 = parse_double(fields[pm25_it->second]);
        }

        auto pm10_it = column_map_.find("pm10");
        if (pm10_it != column_map_.end() && pm10_it->second < fields.size()) {
            record.pm10 = parse_double(fields[pm10_it->second]);
        }

        // Parse optional fields - Noise
        auto db_it = column_map_.find("db");
        if (db_it != column_map_.end() && db_it->second < fields.size()) {
            record.db = parse_double(fields[db_it->second]);
        }

        return record;

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to parse row: ") + e.what());
    }
}

std::chrono::system_clock::time_point ReaderCSV::parse_timestamp(const std::string& ts_str) {
    // Parse ISO 8601 format: 2025-10-13T08:00:00Z
    std::tm tm = {};
    std::istringstream ss(ts_str);
    
    // Try parsing with 'Z' suffix
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    
    if (ss.fail()) {
        throw std::runtime_error("Invalid timestamp format: " + ts_str);
    }

    // Convert to time_point (assuming UTC)
    auto time_c = std::mktime(&tm);
    
    // Adjust for UTC (mktime assumes local time)
    // For simplicity, we'll use the time as-is
    // In production, you'd want to handle timezone properly
    return std::chrono::system_clock::from_time_t(time_c);
}

std::string ReaderCSV::to_lower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::optional<double> ReaderCSV::parse_double(const std::string& str) const {
    if (str.empty()) {
        return std::nullopt;
    }
    
    try {
        size_t pos;
        double value = std::stod(str, &pos);
        
        // Check if entire string was parsed
        if (pos != str.length()) {
            return std::nullopt;
        }
        
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<int> ReaderCSV::parse_int(const std::string& str) const {
    if (str.empty()) {
        return std::nullopt;
    }
    
    try {
        size_t pos;
        int value = std::stoi(str, &pos);
        
        // Check if entire string was parsed
        if (pos != str.length()) {
            return std::nullopt;
        }
        
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace io
