#include "ReaderJSON.hpp"
#include <algorithm>
#include <cctype>

namespace io {

app::Config ReaderJSON::parse_config() {
    std::string json_content = read_file();
    auto json_map = parse_json_object(json_content);

    app::Config config;

    // Parse inputs array
    if (json_map.find("inputs") != json_map.end()) {
        config.inputs = parse_json_array(json_map["inputs"]);
    }

    // Parse output path
    if (json_map.find("out_path") != json_map.end() || json_map.find("out") != json_map.end()) {
        std::string key = json_map.find("out_path") != json_map.end() ? "out_path" : "out";
        config.out_path = extract_string_value(json_map[key]);
    }

    // Parse window_minutes
    if (json_map.find("window_minutes") != json_map.end()) {
        config.window_minutes = extract_int_value(json_map["window_minutes"]);
    }

    // Parse time_step_seconds
    if (json_map.find("time_step_seconds") != json_map.end()) {
        config.time_step_seconds = extract_int_value(json_map["time_step_seconds"]);
    }

    // Parse report_every_ticks
    if (json_map.find("report_every_ticks") != json_map.end()) {
        config.report_every_ticks = extract_int_value(json_map["report_every_ticks"]);
    }

    // Parse detector thresholds
    if (json_map.find("traffic_speed_threshold") != json_map.end()) {
        config.traffic_speed_threshold = extract_double_value(json_map["traffic_speed_threshold"]);
    }

    if (json_map.find("consec_minutes") != json_map.end()) {
        config.consec_minutes = extract_int_value(json_map["consec_minutes"]);
    }

    if (json_map.find("pm25_threshold") != json_map.end()) {
        config.pm25_threshold = extract_double_value(json_map["pm25_threshold"]);
    }

    if (json_map.find("db_threshold") != json_map.end()) {
        config.db_threshold = extract_double_value(json_map["db_threshold"]);
    }

    if (json_map.find("noise_count_threshold") != json_map.end()) {
        config.noise_count_threshold = extract_int_value(json_map["noise_count_threshold"]);
    }

    if (json_map.find("noise_window_minutes") != json_map.end()) {
        config.noise_window_minutes = extract_int_value(json_map["noise_window_minutes"]);
    }

    return config;
}

std::string ReaderJSON::read_file() {
    std::ifstream file(filepath_);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + filepath_);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string ReaderJSON::trim(const std::string& str) const {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    
    if (start == std::string::npos || end == std::string::npos) {
        return "";
    }
    
    return str.substr(start, end - start + 1);
}

size_t ReaderJSON::find_closing(const std::string& str, size_t start, char open, char close) {
    int depth = 1;
    for (size_t i = start + 1; i < str.length(); ++i) {
        if (str[i] == open) {
            depth++;
        } else if (str[i] == close) {
            depth--;
            if (depth == 0) {
                return i;
            }
        }
    }
    throw std::runtime_error("Unmatched bracket/brace in JSON");
}

std::map<std::string, std::string> ReaderJSON::parse_json_object(const std::string& json) {
    std::map<std::string, std::string> result;
    std::string trimmed = trim(json);

    // Find the object boundaries
    size_t obj_start = trimmed.find('{');
    size_t obj_end = trimmed.rfind('}');
    
    if (obj_start == std::string::npos || obj_end == std::string::npos) {
        throw std::runtime_error("Invalid JSON object: missing braces");
    }

    std::string content = trimmed.substr(obj_start + 1, obj_end - obj_start - 1);
    
    // Parse key-value pairs
    size_t pos = 0;
    while (pos < content.length()) {
        // Skip whitespace
        while (pos < content.length() && std::isspace(content[pos])) {
            pos++;
        }
        
        if (pos >= content.length() || content[pos] == '}') {
            break;
        }

        // Find key (quoted string)
        if (content[pos] != '"') {
            pos++;
            continue;
        }
        
        size_t key_start = pos + 1;
        size_t key_end = content.find('"', key_start);
        
        if (key_end == std::string::npos) {
            throw std::runtime_error("Invalid JSON: unterminated string");
        }
        
        std::string key = content.substr(key_start, key_end - key_start);
        pos = key_end + 1;

        // Find colon
        while (pos < content.length() && std::isspace(content[pos])) {
            pos++;
        }
        
        if (pos >= content.length() || content[pos] != ':') {
            throw std::runtime_error("Invalid JSON: missing colon after key");
        }
        pos++; // Skip colon

        // Skip whitespace after colon
        while (pos < content.length() && std::isspace(content[pos])) {
            pos++;
        }

        // Find value
        size_t value_start = pos;
        size_t value_end = pos;

        if (content[pos] == '"') {
            // String value
            value_end = content.find('"', pos + 1);
            if (value_end == std::string::npos) {
                throw std::runtime_error("Invalid JSON: unterminated string value");
            }
            value_end++;
        } else if (content[pos] == '[') {
            // Array value
            value_end = find_closing(content, pos, '[', ']') + 1;
        } else if (content[pos] == '{') {
            // Object value
            value_end = find_closing(content, pos, '{', '}') + 1;
        } else {
            // Number or boolean
            value_end = pos;
            while (value_end < content.length() && 
                   content[value_end] != ',' && 
                   content[value_end] != '}' &&
                   content[value_end] != ']') {
                value_end++;
            }
        }

        std::string value = trim(content.substr(value_start, value_end - value_start));
        result[key] = value;

        pos = value_end;

        // Skip comma
        while (pos < content.length() && (std::isspace(content[pos]) || content[pos] == ',')) {
            pos++;
        }
    }

    return result;
}

std::vector<std::string> ReaderJSON::parse_json_array(const std::string& json) {
    std::vector<std::string> result;
    std::string trimmed = trim(json);

    // Find array boundaries
    size_t arr_start = trimmed.find('[');
    size_t arr_end = trimmed.rfind(']');
    
    if (arr_start == std::string::npos || arr_end == std::string::npos) {
        throw std::runtime_error("Invalid JSON array: missing brackets");
    }

    std::string content = trimmed.substr(arr_start + 1, arr_end - arr_start - 1);
    
    size_t pos = 0;
    while (pos < content.length()) {
        // Skip whitespace
        while (pos < content.length() && std::isspace(content[pos])) {
            pos++;
        }
        
        if (pos >= content.length()) {
            break;
        }

        size_t value_start = pos;
        size_t value_end = pos;

        if (content[pos] == '"') {
            // String value
            value_end = content.find('"', pos + 1);
            if (value_end == std::string::npos) {
                throw std::runtime_error("Invalid JSON array: unterminated string");
            }
            
            std::string value = content.substr(value_start + 1, value_end - value_start - 1);
            result.push_back(value);
            pos = value_end + 1;
        } else if (content[pos] == '[') {
            // Nested array
            value_end = find_closing(content, pos, '[', ']');
            result.push_back(content.substr(value_start, value_end - value_start + 1));
            pos = value_end + 1;
        } else if (content[pos] == '{') {
            // Object
            value_end = find_closing(content, pos, '{', '}');
            result.push_back(content.substr(value_start, value_end - value_start + 1));
            pos = value_end + 1;
        } else {
            // Number or boolean
            while (value_end < content.length() && 
                   content[value_end] != ',' && 
                   content[value_end] != ']') {
                value_end++;
            }
            result.push_back(trim(content.substr(value_start, value_end - value_start)));
            pos = value_end;
        }

        // Skip comma
        while (pos < content.length() && (std::isspace(content[pos]) || content[pos] == ',')) {
            pos++;
        }
    }

    return result;
}

std::string ReaderJSON::extract_string_value(const std::string& json) {
    std::string trimmed = trim(json);
    
    if (trimmed.empty()) {
        return "";
    }
    
    // Remove quotes if present
    if (trimmed.front() == '"' && trimmed.back() == '"') {
        return trimmed.substr(1, trimmed.length() - 2);
    }
    
    return trimmed;
}

int ReaderJSON::extract_int_value(const std::string& json) {
    std::string trimmed = trim(json);
    
    try {
        return std::stoi(trimmed);
    } catch (...) {
        throw std::runtime_error("Invalid integer value: " + trimmed);
    }
}

double ReaderJSON::extract_double_value(const std::string& json) {
    std::string trimmed = trim(json);
    
    try {
        return std::stod(trimmed);
    } catch (...) {
        throw std::runtime_error("Invalid double value: " + trimmed);
    }
}

} // namespace io
