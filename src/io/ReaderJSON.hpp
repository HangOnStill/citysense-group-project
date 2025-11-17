#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <stdexcept>
#include "../app/Config.hpp"

namespace io {

class ReaderJSON {
public:
    explicit ReaderJSON(const std::string& filepath)
        : filepath_(filepath) {}

    app::Config parse_config();

private:
    std::string filepath_;

    // Simple JSON parser helpers
    std::string read_file();
    std::string trim(const std::string& str) const;
    std::map<std::string, std::string> parse_json_object(const std::string& json);
    std::vector<std::string> parse_json_array(const std::string& json);
    std::string extract_string_value(const std::string& json);
    int extract_int_value(const std::string& json);
    double extract_double_value(const std::string& json);
    
    // Helper to find matching brace/bracket
    size_t find_closing(const std::string& str, size_t start, char open, char close);
};

} // namespace io
