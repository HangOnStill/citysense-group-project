#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <stdexcept>
#include "../app/Config.hpp"

using namespace std;

namespace io {

class ReaderJSON {
public:
    explicit ReaderJSON(const string& filepath)
        : filepath_(filepath) {}

    app::Config parse_config();
    
    // Temporarily public for testing
    string read_file();
    string trim(const string& str) const;

private:
    string filepath_;

    // Simple JSON parser helpers
    map<string, string> parse_json_object(const string& json);
    vector<string> parse_json_array(const string& json);
    string extract_string_value(const string& json);
    int extract_int_value(const string& json);
    double extract_double_value(const string& json);
    
    // Helper to find matching brace/bracket
    size_t find_closing(const string& str, size_t start, char open, char close);
};

} // namespace io
