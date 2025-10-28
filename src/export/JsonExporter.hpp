#pragma once
#include <fstream>
#include <string>
#include "../core/Aggregator.hpp"

namespace export_ {

// Minimal JSON-like output without third-party libs.
class JsonExporter {
    std::string path_;
public:
    explicit JsonExporter(std::string path) : path_(std::move(path)) {}

    void emit(const core::Summary& s) {
        std::ofstream out(path_, std::ios::trunc);
        out << "{\n  \"total_count\": " << s.total_count << "\n}\n";
    }
};

} // namespace export_
