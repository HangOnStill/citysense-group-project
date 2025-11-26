#pragma once
#include <string>
#include <vector>
#include <optional>
#include <sstream>

namespace app {

struct CliArgs {
    std::string config_path{"data/config.json"};
    std::vector<std::string> inputs;
    std::vector<int> zones;
    std::optional<int> window_minutes;
    std::optional<std::string> out_path;
    bool help = false;
};

inline void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "Options:\n"
              << "  --config <path>        Path to JSON config file (default: data/config.json)\n"
              << "  --inputs <files>       Comma-separated list of CSV input files\n"
              << "  --zones <ids>          Comma-separated list of zone IDs to filter\n"
              << "  --window <minutes>     Window size in minutes for aggregation\n"
              << "  --out <path>           Output file path for results\n"
              << "  --help                 Show this help message\n";
}

inline std::vector<std::string> split_csv(const std::string& str) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        // Trim whitespace
        size_t start = item.find_first_not_of(" \t");
        size_t end = item.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            result.push_back(item.substr(start, end - start + 1));
        }
    }
    return result;
}

inline CliArgs parse_cli(int argc, char** argv) {
    CliArgs args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            args.help = true;
            return args;
        }
        else if (arg == "--config" && i + 1 < argc) {
            args.config_path = argv[++i];
        }
        else if (arg == "--inputs" && i + 1 < argc) {
            args.inputs = split_csv(argv[++i]);
        }
        else if (arg == "--zones" && i + 1 < argc) {
            auto zone_strs = split_csv(argv[++i]);
            for (const auto& z : zone_strs) {
                try {
                    args.zones.push_back(std::stoi(z));
                } catch (...) {
                    std::cerr << "Warning: Invalid zone ID: " << z << std::endl;
                }
            }
        }
        else if (arg == "--window" && i + 1 < argc) {
            try {
                args.window_minutes = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "Warning: Invalid window value\n";
            }
        }
        else if (arg == "--out" && i + 1 < argc) {
            args.out_path = argv[++i];
        }
    }
    
    return args;
}

} // namespace app
