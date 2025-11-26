# CitySense Development Guide

## Overview
This guide walks you through implementing the CitySense urban sensor data processing system step-by-step. You'll build a complete C++20 application that reads CSV/JSON files, filters data, computes rolling averages, and generates aggregate statistics.

## Project Structure
```
src/
├── io/
│   ├── ReaderCSV.hpp/.cpp      # CSV file parser
│   └── ReaderJSON.hpp/.cpp     # JSON config parser
├── core/
│   └── Processor.hpp/.cpp      # Data processing pipeline
├── app/
│   ├── Cli.hpp                 # Command-line argument parsing
│   ├── Config.hpp              # Configuration structure
│   └── main.cpp                # Application entry point
└── model/
    └── SensorRecord.hpp        # Data structure for sensor readings
```

## Development Steps

### Phase 1: CSV Reader (ReaderCSV)
**Files**: `src/io/ReaderCSV.hpp` and `src/io/ReaderCSV.cpp`

#### Step 1.1: Understand the Data Structure
Look at the CSV files in `data/`:
- `air.csv`: timestamp, sensor_id, zone_id, pm25, pm10
- `noise.csv`: timestamp, sensor_id, zone_id, db
- `traffic.csv`: timestamp, sensor_id, zone_id, speed, flow

All have common fields: timestamp, sensor_id, zone_id
Each has sensor-specific fields (optional in SensorRecord)

#### Step 1.2: Implement Helper Functions First

**a. `split_line()`** - Split CSV line by commas
```cpp
// Algorithm:
// 1. Use std::stringstream with std::getline(ss, field, ',')
// 2. For each field, trim whitespace using find_first_not_of/find_last_not_of
// 3. Add trimmed field to vector
// 4. Return vector

// Example:
// Input:  "A1, 1, 55, 30"
// Output: ["A1", "1", "55", "30"]
```

**b. `to_lower()`** - Convert string to lowercase
```cpp
// Algorithm:
// Use std::transform with std::tolower

// Example:
// Input:  "Sensor_ID"
// Output: "sensor_id"
```

**c. `parse_double()` and `parse_int()`** - Parse numbers safely
```cpp
// Algorithm:
// 1. Check if string is empty -> return nullopt
// 2. Try std::stod/std::stoi with size_t* pos parameter
// 3. Check if pos equals string length (entire string parsed)
// 4. Return value or nullopt

// Example:
// parse_double("42.5") -> optional(42.5)
// parse_double("abc")  -> nullopt
```

**d. `parse_timestamp()`** - Parse ISO 8601 timestamp
```cpp
// Algorithm:
// 1. Create std::tm struct (initialize to {})
// 2. Use std::istringstream and std::get_time
//    Format: "%Y-%m-%dT%H:%M:%S"
// 3. Check if parsing failed (ss.fail())
// 4. Convert tm to time_t using std::mktime
// 5. Convert time_t to system_clock::time_point
// 6. Return time_point

// Example:
// Input: "2025-10-13T08:00:00Z"
// Output: chrono::system_clock::time_point
```

#### Step 1.3: Implement CSV Header Parsing

**`parse_header()`** - Build column name mapping
```cpp
// Algorithm:
// 1. Split line using split_line()
// 2. For each field (with index i):
//    a. Convert to lowercase (to_lower)
//    b. Remove whitespace
//    c. Store in column_map_: map[normalized_name] = i
// 3. Validate required columns exist:
//    - "timestamp"
//    - "sensor_id" or "sensorid"
//    - "zone_id" or "zoneid"
// 4. Throw error if any required column missing

// Example:
// Input: "Timestamp, Sensor_ID, Zone_ID, PM2.5, PM10"
// Result: column_map_ = {
//   "timestamp" -> 0,
//   "sensor_id" -> 1,
//   "zone_id" -> 2,
//   "pm2.5" -> 3,
//   "pm10" -> 4
// }
```

#### Step 1.4: Implement Row Parsing

**`parse_row()`** - Convert CSV row to SensorRecord
```cpp
// Algorithm:
// 1. Create empty SensorRecord
// 2. Parse REQUIRED fields:
//    a. Find "timestamp" in column_map_, parse with parse_timestamp()
//    b. Find "sensor_id", get string directly
//    c. Find "zone_id", parse with parse_int()
//    d. If any required field missing/invalid -> return nullopt
// 3. Parse OPTIONAL fields (check if column exists first):
//    - "speed" -> record.speed = parse_double(...)
//    - "flow" -> record.flow = parse_double(...)
//    - "pm25" or "pm2.5" -> record.pm25 = parse_double(...)
//    - "pm10" -> record.pm10 = parse_double(...)
//    - "db" -> record.db = parse_double(...)
// 4. Return record

// Example:
// Input fields: ["2025-10-13T08:00:00Z", "A1", "1", "22", "30"]
// Output: SensorRecord{
//   ts = <time_point>,
//   sensor_id = "A1",
//   zone_id = 1,
//   pm25 = 22.0,
//   pm10 = 30.0
// }
```

#### Step 1.5: Implement File Management

**`open_next_file()`** - Open next CSV file
```cpp
// Algorithm:
// 1. If current_stream_ is open, close it
// 2. Check if current_file_index_ >= inputs_.size() -> return (no more files)
// 3. Get filepath = inputs_[current_file_index_]
// 4. Open file: current_stream_.open(filepath)
// 5. Check if open failed -> throw runtime_error
// 6. Reset state: header_parsed_ = false, column_map_.clear()
```

**`next_batch()`** - Read batch of records
```cpp
// Algorithm:
// 1. Create empty vector<SensorRecord>
// 2. Loop while rows_read < max_rows:
//    a. Try to read line from current_stream_
//    b. If EOF (getline fails):
//       - Increment current_file_index_
//       - Call open_next_file()
//       - If no more files, break
//       - Continue to next iteration
//    c. Skip empty lines
//    d. If !header_parsed_:
//       - Call parse_header(line)
//       - Set header_parsed_ = true
//       - Continue
//    e. Otherwise (data row):
//       - Increment stats_.total_rows
//       - Split line with split_line()
//       - Call parse_row(fields)
//       - If result.has_value():
//         * Add to vector
//         * Increment stats_.parsed_rows
//       - Else: increment stats_.malformed_rows
// 3. Return vector
```

#### Step 1.6: Test Your CSV Reader
```bash
# Compile test
g++ -std=c++20 -I. test_readers.cpp src/io/ReaderCSV.cpp -o test_csv.exe

# Run test
./test_csv.exe
```

Expected output:
- Air CSV Records: 7 records with pm25/pm10 values
- Noise CSV Records: 7 records with db values
- Traffic CSV Records: 7 records with speed/flow values

---

### Phase 2: JSON Config Reader (ReaderJSON)
**Files**: `src/io/ReaderJSON.hpp` and `src/io/ReaderJSON.cpp`

#### Step 2.1: Understand JSON Structure
Look at `data/config.json`:
```json
{
  "inputs": ["data/air.csv"],
  "window_minutes": 5
}
```

#### Step 2.2: Implement Helper Functions

**a. `trim()`** - Remove whitespace
```cpp
// Already explained above
```

**b. `read_file()`** - Read entire file
```cpp
// Algorithm:
// 1. Create std::ifstream file(filepath_)
// 2. Check if file.is_open() -> throw error if not
// 3. Create std::stringstream buffer
// 4. Read entire file: buffer << file.rdbuf()
// 5. Return buffer.str()
```

**c. `find_closing()`** - Find matching bracket/brace
```cpp
// Algorithm:
// 1. Initialize depth = 1
// 2. Loop from start+1 to str.length():
//    a. If str[i] == open: depth++
//    b. If str[i] == close: depth--
//    c. If depth == 0: return i
// 3. If loop finishes: throw error (unmatched)

// Example:
// str = "[1, [2, 3], 4]"
// find_closing(str, 0, '[', ']') -> returns 13 (last ']')
```

#### Step 2.3: Implement JSON Parsing

**`parse_json_object()`** - Parse JSON object to map
```cpp
// Algorithm:
// 1. Trim input, find '{' and '}'
// 2. Extract content between braces
// 3. Initialize pos = 0
// 4. Loop while pos < content.length():
//    a. Skip whitespace
//    b. Find key:
//       - Look for '"'
//       - Find closing '"'
//       - Extract key string
//    c. Find colon ':' after key
//    d. Skip whitespace after colon
//    e. Determine value type and extract:
//       - If '"': string (find closing '"')
//       - If '[': array (use find_closing)
//       - If '{': object (use find_closing)
//       - Otherwise: number/boolean (read until ',', '}', or ']')
//    f. Store in map: result[key] = value
//    g. Skip comma if present
// 5. Return map

// Example:
// Input: '{"name": "test", "value": 42}'
// Output: map{"name" -> "\"test\"", "value" -> "42"}
```

**`parse_json_array()`** - Parse JSON array to vector
```cpp
// Algorithm:
// Similar to parse_json_object but:
// 1. Find '[' and ']' instead of '{' and '}'
// 2. Extract elements instead of key-value pairs
// 3. For string elements, strip quotes before adding to vector
```

**`extract_string_value()`** - Remove quotes from string
```cpp
// Algorithm:
// 1. Trim input
// 2. Check if starts with '"' and ends with '"'
// 3. If yes, return substr(1, length-2)
// 4. Otherwise return as-is
```

**`extract_int_value()` and `extract_double_value()`**
```cpp
// Algorithm:
// 1. Trim input
// 2. Use std::stoi / std::stod
// 3. Catch exceptions and rethrow with better message
```

#### Step 2.4: Implement Config Parsing

**`parse_config()`** - Main parsing function
```cpp
// Algorithm:
// 1. Read file: json_content = read_file()
// 2. Parse object: json_map = parse_json_object(json_content)
// 3. Create Config struct
// 4. For each possible field:
//    if (json_map.find("field_name") != json_map.end()) {
//        config.field = extract_appropriate_type(json_map["field_name"]);
//    }
// 5. Return config

// Fields to handle:
// - inputs: parse_json_array -> vector<string>
// - out_path / out: extract_string_value -> string
// - window_minutes: extract_int_value -> int
// - time_step_seconds: extract_int_value -> int
// - traffic_speed_threshold: extract_double_value -> double
// - pm25_threshold: extract_double_value -> double
// - db_threshold: extract_double_value -> double
// - etc.
```

#### Step 2.5: Test Your JSON Reader
```bash
# Compile and run
g++ -std=c++20 -I. test_readers.cpp src/io/ReaderJSON.cpp -o test_json.exe
./test_json.exe
```

---

### Phase 3: Processing Pipeline (Processor)
**Files**: `src/core/Processor.hpp` and `src/core/Processor.cpp`

#### Step 3.1: Understand the Pipeline
The processor:
1. Filters records by time and zone
2. Applies rolling averages per sensor
3. Groups records into time buckets
4. Computes aggregate statistics

#### Step 3.2: Implement Filtering

**`passes_filters()`** - Check if record passes filters
```cpp
// Algorithm:
// 1. Check time filters:
//    if (config_.start_time.has_value() && record.ts < start_time)
//        -> increment filtered_by_time, return false
//    if (config_.end_time.has_value() && record.ts > end_time)
//        -> increment filtered_by_time, return false
// 2. Check zone filter:
//    if (!config_.zone_filter.empty())
//        if (zone_id not in zone_filter)
//            -> increment filtered_by_zone, return false
// 3. Return true (passed all filters)
```

#### Step 3.3: Implement Time Bucketing

**`get_bucket_start()`** - Calculate bucket start time
```cpp
// Algorithm:
// 1. Convert timestamp to minutes since epoch:
//    auto duration = timestamp.time_since_epoch()
//    auto minutes = duration_cast<minutes>(duration).count()
// 2. Round down to bucket boundary:
//    bucket_minutes = (minutes / config_.bucket_minutes) * config_.bucket_minutes
// 3. Convert back to time_point:
//    return time_point(chrono::minutes(bucket_minutes))

// Example:
// timestamp = 08:07, bucket_minutes = 5
// -> 487 minutes / 5 = 97
// -> 97 * 5 = 485 minutes
// -> 485 minutes = 08:05
```

#### Step 3.4: Implement Rolling Averages

**`apply_transformations()`** - Apply rolling averages
```cpp
// Algorithm:
// 1. Create copy of record
// 2. If !config_.apply_rolling_avg: return copy
// 3. For each optional field (speed, flow, pm25, pm10, db):
//    if (record.field.has_value()) {
//        // Get or create rolling average tracker for this sensor
//        auto& rolling = field_rolling_[record.sensor_id];
//        if (rolling.max_size == 0)
//            rolling.max_size = config_.rolling_window_size;
//        
//        // Add new value to window
//        rolling.add(record.field.value());
//        
//        // Replace with rolling average
//        transformed.field = rolling.get_average();
//    }
// 4. Return transformed record
```

#### Step 3.5: Implement Batch Processing

**`process_batch()`** - Main processing function
```cpp
// Algorithm:
// 1. Loop through each record:
//    a. Increment diag_.total_records
//    b. Check if passes_filters() -> skip if not
//    c. Apply transformations: transformed = apply_transformations(record)
//    d. Get bucket: bucket_start = get_bucket_start(transformed.ts)
//    e. Create bucket key: {bucket_start, transformed.zone_id}
//    f. Add values to bucket:
//       if (transformed.speed.has_value())
//           buckets_[key].speed_values.push_back(transformed.speed.value())
//       // Repeat for flow, pm25, pm10, db
//    g. Increment diag_.processed
```

#### Step 3.6: Implement Statistics Computation

**`compute_stats()`** - Calculate aggregates
```cpp
// Algorithm:
// 1. If values.empty(): return (leave optionals empty)
// 2. Calculate mean:
//    sum = std::accumulate(values.begin(), values.end(), 0.0)
//    mean = sum / values.size()
// 3. Sort values for percentiles:
//    vector<double> sorted = values
//    std::sort(sorted.begin(), sorted.end())
// 4. Calculate median:
//    size_t mid = sorted.size() / 2
//    if (sorted.size() % 2 == 0)
//        median = (sorted[mid-1] + sorted[mid]) / 2.0
//    else
//        median = sorted[mid]
// 5. Calculate percentiles:
//    idx_90 = ceil(0.90 * sorted.size()) - 1
//    p90 = sorted[idx_90]
//    idx_99 = ceil(0.99 * sorted.size()) - 1
//    p99 = sorted[idx_99]
```

**`get_bucket_stats()`** - Generate statistics for all buckets
```cpp
// Algorithm:
// 1. Create empty vector<BucketStats>
// 2. Loop through buckets_ map:
//    for (const auto& [key, data] : buckets_) {
//        BucketStats stats;
//        stats.bucket_start = key.bucket_start;
//        stats.zone_id = key.zone_id;
//        stats.count = // sum of all value vector sizes
//        
//        // Compute stats for each metric
//        compute_stats(data.speed_values, stats.speed_mean,
//                     stats.speed_median, stats.speed_p90, stats.speed_p99);
//        // Repeat for flow, pm25, pm10, db
//        
//        results.push_back(stats);
//    }
// 3. Return results
```

---

### Phase 4: CLI and Integration

#### Step 4.1: CLI is already implemented in `src/app/Cli.hpp`
Review the code to understand:
- How arguments are parsed
- How config is overridden by CLI flags

#### Step 4.2: Create Main Application
Create `my_main.cpp`:
```cpp
#include <iostream>
#include "src/io/ReaderCSV.hpp"
#include "src/io/ReaderJSON.hpp"
#include "src/core/Processor.hpp"
#include "src/app/Cli.hpp"

int main(int argc, char** argv) {
    // 1. Parse CLI arguments
    auto cli_args = app::parse_cli(argc, argv);
    
    // 2. Load JSON config
    io::ReaderJSON json_reader(cli_args.config_path);
    auto config = json_reader.parse_config();
    
    // 3. Override config with CLI args
    if (!cli_args.inputs.empty())
        config.inputs = cli_args.inputs;
    // ... other overrides
    
    // 4. Setup processor
    core::Processor::Config proc_config;
    proc_config.zone_filter = cli_args.zones;
    proc_config.bucket_minutes = config.window_minutes;
    proc_config.apply_rolling_avg = true;
    core::Processor processor(proc_config);
    
    // 5. Read and process CSV files
    io::ReaderCSV csv_reader(config.inputs);
    while (true) {
        auto batch = csv_reader.next_batch(100);
        if (batch.empty()) break;
        processor.process_batch(batch);
    }
    
    // 6. Get and display results
    auto stats = processor.get_bucket_stats();
    for (const auto& bucket : stats) {
        std::cout << "Zone " << bucket.zone_id << ": ";
        if (bucket.speed_mean.has_value())
            std::cout << "Speed avg=" << bucket.speed_mean.value();
        std::cout << "\n";
    }
    
    return 0;
}
```

---

## Building and Testing

### Build Commands
```bash
# Build CSV reader test
g++ -std=c++20 -I. test_readers.cpp src/io/ReaderCSV.cpp -o test_csv.exe

# Build JSON reader test
g++ -std=c++20 -I. test_readers.cpp src/io/ReaderJSON.cpp -o test_json.exe

# Build full application
g++ -std=c++20 -I. my_main.cpp src/io/ReaderCSV.cpp src/io/ReaderJSON.cpp src/core/Processor.cpp -o citysense.exe

# Run comprehensive tests
g++ -std=c++20 -I. run_tests.cpp src/io/ReaderCSV.cpp src/io/ReaderJSON.cpp src/core/Processor.cpp -o run_tests.exe
./run_tests.exe
```

### Testing Strategy
1. **Unit test each component**:
   - Test CSV parser with sample data
   - Test JSON parser with config file
   - Test processor with known inputs

2. **Integration test**:
   - Run full pipeline with all sensor types
   - Verify filtering works
   - Check aggregate calculations

3. **Edge cases**:
   - Empty files
   - Malformed CSV rows
   - Missing required fields
   - Invalid timestamps

---

## Debugging Tips

1. **CSV parsing issues**:
   - Print column_map_ after parse_header()
   - Check field indices match data

2. **Timestamp parsing**:
   - Test with simple timestamp first
   - Check tm struct values after parsing

3. **Rolling averages**:
   - Print values before/after transformation
   - Verify window size is correct

4. **Statistics**:
   - Test compute_stats with known values
   - Example: [1, 2, 3, 4, 5] should give mean=3, median=3

---

## Common Pitfalls

1. **Off-by-one errors** in percentile calculation
2. **Forgetting to handle optional fields** (check has_value())
3. **Not clearing state** when opening new file
4. **Case sensitivity** in column names
5. **Whitespace** in CSV fields
6. **Integer overflow** in timestamp calculations

---

## Next Steps After Implementation

1. Add JSON output exporter
2. Implement actual detector algorithms
3. Add multi-threading for large files
4. Create visualization tools
5. Add unit tests for edge cases

Good luck! 🚀
