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
using namespace std;
namespace io {

/**
 * CSV Reader for sensor data files
 * 
 * PURPOSE:
 * - Reads CSV files containing sensor data (air quality, noise, traffic)
 * - Supports multiple input files processed sequentially
 * - Case-insensitive header parsing
 * - Handles optional fields based on sensor type
 * - Tracks parsing statistics and errors
 * 
 * USAGE:
 *   ReaderCSV reader({"data/air.csv", "data/traffic.csv"});
 *   auto batch = reader.next_batch(100); // Read up to 100 records
 *   auto stats = reader.get_stats();     // Get parsing statistics
 */
class ReaderCSV {
public:
    /**
     * Constructor: Initialize CSV reader with list of input files
     * @param inputs Vector of file paths to read
     * @throws std::invalid_argument if inputs is empty
     * @throws std::runtime_error if first file cannot be opened
     */
    explicit ReaderCSV(vector<std::string> inputs)
      : inputs_(move(inputs)), current_file_index_(0) {
        if (inputs_.empty()) {
            throw std::invalid_argument("No input files provided");
        }
        open_next_file();
    }

    /**
     * Read the next batch of records from CSV files
     * @param max_rows Maximum number of records to return (default: 1000)
     * @return Vector of parsed SensorRecord objects
     * 
     * NOTE: Automatically moves to next file when current file is exhausted
     */
    vector<model::SensorRecord> next_batch(std::size_t max_rows = 1000);

    /**
     * Statistics structure for tracking CSV parsing results
     * - total_rows: Total data rows encountered (excludes header)
     * - parsed_rows: Successfully parsed rows
     * - malformed_rows: Rows that failed parsing
     * - errors: List of error messages for debugging
     */
    struct ParseStats {
        size_t total_rows = 0;
        size_t parsed_rows = 0;
        size_t malformed_rows = 0;
        std::vector<std::string> errors;
    };

    /**
     * Get current parsing statistics
     * @return Reference to internal ParseStats structure
     */
    const ParseStats& get_stats() const { return stats_; }

private:
    // Member variables
    std::vector<std::string> inputs_;           // List of CSV files to process
    size_t current_file_index_;                 // Current file being processed
    std::ifstream current_stream_;              // File stream for current file
    std::map<std::string, size_t> column_map_;  // Maps column names to indices
    bool header_parsed_ = false;                // Flag for header processing
    ParseStats stats_;                          // Accumulated parsing statistics

    // Private helper methods
    
    /**
     * Open the next file in the inputs_ list
     * @throws std::runtime_error if file cannot be opened
     */
    void open_next_file();
    
    /**
     * Parse CSV header line and build column mapping
     * Headers are normalized to lowercase for case-insensitive matching
     * @param line The header line from CSV
     * @throws std::runtime_error if required columns are missing
     */
    void parse_header(const std::string& line);
    
    /**
     * Split a CSV line into fields
     * @param line Raw CSV line
     * @return Vector of trimmed field values
     */
    std::vector<std::string> split_line(const std::string& line) const;
    
    /**
     * Parse a row of CSV data into a SensorRecord
     * @param fields Vector of field values from CSV
     * @return SensorRecord if successful, nullopt if parsing fails
     */
    std::optional<model::SensorRecord> parse_row(const std::vector<std::string>& fields);
    
    /**
     * Parse ISO 8601 timestamp (e.g., "2025-10-13T08:00:00Z")
     * @param ts_str Timestamp string
     * @return std::chrono time_point
     * @throws std::runtime_error if format is invalid
     */
    std::chrono::system_clock::time_point parse_timestamp(const std::string& ts_str);
    
    /**
     * Convert string to lowercase for case-insensitive comparison
     * @param str Input string
     * @return Lowercase version of input
     */
    std::string to_lower(const std::string& str) const;
    
    /**
     * Safely parse a string to double
     * @param str String to parse
     * @return Double value if valid, nullopt otherwise
     */
    std::optional<double> parse_double(const std::string& str) const;
    
    /**
     * Safely parse a string to int
     * @param str String to parse
     * @return Int value if valid, nullopt otherwise
     */
    std::optional<int> parse_int(const std::string& str) const;
};

} // namespace io
