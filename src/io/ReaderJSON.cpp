#include "ReaderJSON.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <fstream>



using namespace std;
using json = nlohmann::json;

namespace io {



/*
 * FUNCTION: parse_config
 * PURPOSE: Main function to parse JSON config file into Config struct
 * RETURNS: app::Config with all settings
 * 
 * TODO - Implement this function:
 * 1. Read entire JSON file (use read_file())
 * 2. Parse JSON object (use parse_json_object())
 * 3. Create Config struct
 * 4. For each possible field in JSON:
 *    - Check if key exists in json_map
 *    - Parse value using appropriate extract_* function
 *    - Set in config struct
 * 5. Return config
 * 
 * Fields to parse:
 * - inputs (array of strings)
 * - out_path or out (string)
 * - window_minutes (int)
 * - time_step_seconds (int)
 * - report_every_ticks (int)
 * - traffic_speed_threshold (double)
 * - consec_minutes (int)
 * - pm25_threshold (double)
 * - db_threshold (double)
 * - noise_count_threshold (int)
 * - noise_window_minutes (int)
 */
app::Config ReaderJSON::parse_config() {
    
    ifstream file(filepath_);
    if(!file.is_open()){
        throw runtime_error("Failed to open JSON file: " + filepath_);
    }
        json j;
        try{
            j = json::parse(read_file());

        } catch(const json::parse_error& e){
            throw runtime_error("JSON parse error: " + string(e.what()));
        }

        app::Config config;

        if(j.contains("inputs") && j["inputs"].is_array()){
            config.inputs = j["inputs"].get<vector<string>>();
        }
        if(j.contains("window_minutes")){
            config.window_minutes = j["window_minutes"].get<int>();
        }

        if(j.contains("out") || j.contains("out_path")){
            config.out_path = j.contains("out") ? 
                j["out"].get<string>() : 
                j["out_path"].get<string>();
        }
        if(j.contains("time_step_seconds")){
            config.time_step_seconds = j["time_step_seconds"].get<int>();
        }
        if(j.contains("report_every_ticks") && j["report_every_ticks"].is_number()){
            config.report_every_ticks = j["report_every_ticks"].get<int>();
        }
        if(j.contains("traffic_speed_threshold") && j["traffic_speed_threshold"].is_number()){
            config.traffic_speed_threshold = j["traffic_speed_threshold"].get<double>();
        }
        if(j.contains("consec_minutes") && j["consec_minutes"].is_number()){
            config.consec_minutes = j["consec_minutes"].get<int>();
        }
        if(j.contains("pm25_threshold") && j["pm25_threshold"].is_number()){
            config.pm25_threshold = j["pm25_threshold"].get<double>();
        }
        if(j.contains("db_threshold") && j["db_threshold"].is_number()){
            config.db_threshold = j["db_threshold"].get<double>();
        }
        if(j.contains("noise_count_threshold") && j["noise_count_threshold"].is_number()){
            config.noise_count_threshold = j["noise_count_threshold"].get<int>();
        }
        if(j.contains("noise_window_minutes") && j["noise_window_minutes"].is_number()){
            config.noise_window_minutes = j["noise_window_minutes"].get<int>();
        }

        return config;
}

/*
 * FUNCTION: read_file
 * PURPOSE: Read entire JSON file into string
 * RETURNS: string containing file contents
 * 
 * TODO - Implement this function:
 * 1. Open file using filepath_
 * 2. Check if file opened successfully
 * 3. Read entire file into stringstream
 * 4. Return as string
 * 5. Throw error if file can't be opened
 */
string ReaderJSON::read_file() {
    ifstream file(filepath_);

    if(!file.is_open()){
        throw runtime_error("Failed to open JSON file: " + filepath_);
    }
    //reads entire file into stringstream buffer
    stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

/*
 * FUNCTION: trim
 * PURPOSE: Remove leading and trailing whitespace
 * PARAMETERS: str - string to trim
 * RETURNS: trimmed string
 * 
 * TODO - Implement this function:
 * 1. Find first non-whitespace character
 * 2. Find last non-whitespace character
 * 3. Return substring between them
 */
 string ReaderJSON::trim(const std::string& str) const {

    // find_first_not_of(" \t"); finds the first character that isn't a space
    size_t start = str.find_first_not_of(" \t\r\n");

    size_t end = str.find_last_not_of(" \t\r\n");

    if(start == string::npos){
        return "";
    }

    return str.substr(start, end - start + 1);
}

}

 // namespace io




