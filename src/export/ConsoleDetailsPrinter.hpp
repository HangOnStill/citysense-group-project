#pragma once
#include <iostream>
#include <vector>
#include <string>

#include "../detectors/Finding.hpp"

namespace export_ {

    class ConsoleDetailsPrinter {
    public:
        void emit(const std::vector<detectors::Finding>& findings) const {
            if (findings.empty()) {
                std::cout << "[details] No detector findings.\n";
                return;
            }

            std::cout << "[details] Detector Findings:\n";
            for (const auto& f : findings) {
                std::cout << "  - [" << f.detector << "] "
                    << "zone " << f.zone_id << ": "
                    << f.description;

                if (f.metric_value) {
                    std::cout << " (value=" << *f.metric_value << ")";
                }

                std::cout << "\n";
            }
        }
    };

} // namespace export_
