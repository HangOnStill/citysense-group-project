#include "ReaderCSV.hpp"
#include <iostream>

using namespace std;
namespace io {



void ReaderCSV::open_next_file(){
    if(current_file_index_ >= inputs_.size()){
        throw std::runtime_error("No more input files");
    }

    if(current_stream_.is_open()){
        current_stream_.close();
    }

    // Actually open the file
    current_stream_.open(inputs_[current_file_index_]);
    
    if(!current_stream_.is_open()){
        throw runtime_error("Failed to open file " + inputs_[current_file_index_]);
    }

    header_parsed_ = false;
    column_map_.clear();
}    



void ReaderCSV::parse_header(const string& line){
    column_map_.clear();

    stringstream ss(line);
    string col;
    size_t index = 0;

    while(getline(ss, col, ',')){

        col.erase(0, col.find_first_not_of(" \t"));
        col.erase(col.find_last_not_of(" \t") + 1);
        transform(col.begin(), col.end(), col.begin(), ::tolower);
        column_map_[col] = index++;
    }

    const vector<string> required = {"sensor_id", "zone_id", "timestamp"};

    for(const auto& r : required){
        if(!column_map_.count(r)){
            throw runtime_error("Missing required column: " + r);
        }
    }
}

vector<string> ReaderCSV::split_line(const string& line) const {
    vector<string> fields;
    string field;
    stringstream ss(line);

    while (getline(ss, field, ',')) {
        // trim
        field.erase(0, field.find_first_not_of(" \t"));
        field.erase(field.find_last_not_of(" \t") + 1);
        fields.push_back(field);
    }

    return fields;
}
optional<model::SensorRecord> ReaderCSV::parse_row(const vector<string>& fields) {
    // Basic validation: must have at least all known columns.
    if (fields.size() < column_map_.size()) {
        return nullopt;
    }

    model::SensorRecord rec;

    try {
        // Required: sensor_id
        rec.sensor_id = fields[column_map_.at("sensor_id")];

        // Required: zone_id
        auto zid = parse_int(fields[column_map_.at("zone_id")]);
        if (!zid) return nullopt;
        rec.zone_id = *zid;

        // Required: timestamp
        rec.ts = parse_timestamp(fields[column_map_.at("timestamp")]);

        // Optional fields (Traffic, Air, Noise)
        if (column_map_.count("speed")) {
            auto v = parse_double(fields[column_map_.at("speed")]);
            if (v) rec.speed = *v;
        }

        if (column_map_.count("flow")) {
            auto v = parse_double(fields[column_map_.at("flow")]);
            if (v) rec.flow = *v;
        }

        if (column_map_.count("pm25")) {
            auto v = parse_double(fields[column_map_.at("pm25")]);
            if (v) rec.pm25 = *v;
        }

        if (column_map_.count("pm10")) {
            auto v = parse_double(fields[column_map_.at("pm10")]);
            if (v) rec.pm10 = *v;
        }

        if (column_map_.count("db")) {
            auto v = parse_double(fields[column_map_.at("db")]);
            if (v) rec.db = *v;
        }

    } catch (...) {
        return nullopt;  // Any parsing error → malformed row
    }

    return rec;
}




/*
 * FUNCTION: next_batch
 * PURPOSE: Read and parse the next batch of CSV records
 * PARAMETERS: max_rows - maximum number of records to read
 * RETURNS: vector of SensorRecord objects
 * 
 * TODO - Implement this function:
 * 1. Create empty vector to store records
 * 2. Loop until max_rows read or no more data:
 *    a. Read line from current_stream_
 *    b. If EOF, move to next file (open_next_file)
 *    c. Skip empty lines
 *    d. If first line, parse as header
 *    e. Otherwise, parse as data row
 *    f. Update statistics
 * 3. Return the vector of records
 */
vector<model::SensorRecord> ReaderCSV::next_batch(size_t max_rows) {
    vector<model::SensorRecord> records;
    string line;

    while (records.size() < max_rows) {

        // Try to read a line
        if (!std::getline(current_stream_, line)) {

            // Move to next file
            current_file_index_++;
            if (current_file_index_ >= inputs_.size()) {
                break; // No more files
            }

            open_next_file();
            continue;
        }

        // Skip empty lines
        if (line.find_first_not_of(" \t") == string::npos)
            continue;

        // Parse header if needed
        if (!header_parsed_) {
            try {
                parse_header(line);
                header_parsed_ = true;
            } catch (const std::exception& e) {
                stats_.errors.push_back(e.what());
            }
            continue;
        }

        // Split line into fields
        auto fields = split_line(line);
        stats_.total_rows++;

        // Parse row into SensorRecord
        auto rec_opt = parse_row(fields);

        if (!rec_opt) {
            stats_.malformed_rows++;
            stats_.errors.push_back("Malformed row: " + line);
            continue;
        }

        stats_.parsed_rows++;
        records.push_back(std::move(*rec_opt));
    }

    return records;
}

// Parse timestamp from ISO 8601 format (e.g., "2025-10-13T08:00:00Z")
chrono::system_clock::time_point ReaderCSV::parse_timestamp(const string& ts_str) {
    tm tm = {};
    istringstream ss(ts_str);
    ss >> get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    
    if (ss.fail()) {
        throw runtime_error("Invalid timestamp format: " + ts_str);
    }
    
    time_t time = mktime(&tm);
    return chrono::system_clock::from_time_t(time);
}

// Parse string to double
optional<double> ReaderCSV::parse_double(const string& str) const {
    if (str.empty()) {
        return nullopt;
    }
    
    try {
        size_t pos = 0;
        double value = stod(str, &pos);
        
        // Make sure entire string was parsed
        if (pos != str.length()) {
            return nullopt;
        }
        
        return value;
    } catch (...) {
        return nullopt;
    }
}

// Parse string to int
optional<int> ReaderCSV::parse_int(const string& str) const {
    if (str.empty()) {
        return nullopt;
    }
    
    try {
        size_t pos = 0;
        int value = stoi(str, &pos);
        
        // Make sure entire string was parsed
        if (pos != str.length()) {
            return nullopt;
        }
        
        return value;
    } catch (...) {
        return nullopt;
    }
}

string ReaderCSV::to_lower(const string& str) const {
    string result = str;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

} // namespace io
