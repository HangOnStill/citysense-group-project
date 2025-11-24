#pragma once
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>

#include "../model/SensorRecord.hpp"

namespace io {

// Minimal contract only. Students must implement real parsing.
class ReaderCSV {
 public:
  explicit ReaderCSV(std::vector<std::string> inputs)
      : inputs_(std::move(inputs)) {}

  std::vector<model::SensorRecord> next_batch(std::size_t max_rows = 1000) {
    // Parse CSV headers, map to SensorRecord fields, return batch

    std::vector<model::SensorRecord> records;
    std::size_t produced = 0;

    for (std::string input : inputs_) {
      if (input.empty()) {
        throw std::runtime_error("Input file path is empty");
      }

      // Try path as-is, then try ../data/filename
      std::string path = input;
      if (!std::filesystem::exists(path)) {
        std::filesystem::path alt("../");
        alt /= input;
        if (std::filesystem::exists(alt)) {
          path = alt.string();
        }
      }

      std::ifstream file(path);
      if (!file.is_open()) {
        throw std::runtime_error("Input file does not exist: " + input);
      }

      std::string header_line;
      std::getline(file, header_line);  // Read and parse header

      // Parse header to find column indices
      std::vector<std::string> header_fields;
      std::istringstream hss(header_line);
      std::string token;
      while (std::getline(hss, token, ',')) {
        header_fields.push_back(token);
      }

      // Build column index map
      std::unordered_map<std::string, size_t> col_idx;
      for (size_t i = 0; i < header_fields.size(); ++i) {
        col_idx[header_fields[i]] = i;
      }

      while (produced < max_rows && std::getline(file, header_line)) {
        model::SensorRecord record;
        std::vector<std::string> fields;
        fields.reserve(6);
        std::istringstream ss(header_line);

        // Parse CSV line into fields
        while (std::getline(ss, token, ',')) {
          fields.push_back(token);
        }

        try {
          if (fields.size() >= 3) {
            // Parse timestamp (ISO8601 format: YYYY-MM-DDTHH:MM:SSZ)
            if (col_idx.count("timestamp")) {
              size_t ti = col_idx["timestamp"];
              if (ti < fields.size()) {
                std::string ts_str = fields[ti];
                std::istringstream tss(ts_str);
                std::chrono::sys_seconds tp;
                tss >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", tp);
                if (tss.fail()) {
                  continue;  // skip row if timestamp parsing fails
                }
                record.ts = tp;
              }
            }

            // Parse sensor_id
            if (col_idx.count("sensor_id")) {
              size_t si = col_idx["sensor_id"];
              if (si < fields.size()) {
                record.sensor_id = fields[si];
              }
            }

            // Parse zone_id
            if (col_idx.count("zone_id")) {
              size_t zi = col_idx["zone_id"];
              if (zi < fields.size()) {
                record.zone_id = std::stoi(fields[zi]);
              }
            }

            // Parse optional fields based on header presence
            if (col_idx.count("pm25")) {
              size_t idx = col_idx["pm25"];
              if (idx < fields.size() && !fields[idx].empty()) {
                record.pm25 = std::stod(fields[idx]);
              }
            }
            if (col_idx.count("pm10")) {
              size_t idx = col_idx["pm10"];
              if (idx < fields.size() && !fields[idx].empty()) {
                record.pm10 = std::stod(fields[idx]);
              }
            }
            if (col_idx.count("db")) {
              size_t idx = col_idx["db"];
              if (idx < fields.size() && !fields[idx].empty()) {
                record.db = std::stod(fields[idx]);
              }
            }
            if (col_idx.count("speed")) {
              size_t idx = col_idx["speed"];
              if (idx < fields.size() && !fields[idx].empty()) {
                record.speed = std::stod(fields[idx]);
              }
            }
            if (col_idx.count("flow")) {
              size_t idx = col_idx["flow"];
              if (idx < fields.size() && !fields[idx].empty()) {
                record.flow = std::stod(fields[idx]);
              }
            }

            records.push_back(record);
            ++produced;
          }
        } catch (const std::exception& e) {
          continue;  // Skip malformed rows
        }
      }
      file.close();
      if (produced >= max_rows) break;
    }
    return records;
  }

 private:
  std::vector<std::string> inputs_;
};  // namespace io

}  // namespace io
