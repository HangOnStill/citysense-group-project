#pragma once
#include <iostream>
#include "../core/Aggregator.hpp"

namespace export_ {

struct ConsolePrinter {
    void emit(const core::Summary& s) {
        std::cout << "[summary] total_count=" << s.total_count << "\n";
    }
};

} // namespace export_
