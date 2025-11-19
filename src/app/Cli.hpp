#pragma once
#include <string>
#include <cstddef>
#include <cstdlib>

namespace app {

    struct CliArgs {
        std::string config_path{ "data/config.json" };
        std::size_t batch_size{ 1000 };
        std::size_t reserve_records{ 0 };
    };

    inline CliArgs parse_cli(int argc, char** argv) {
        CliArgs a;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--config" && i + 1 < argc) {
                a.config_path = argv[++i];
            }
            else if (arg == "--batch" && i + 1 < argc) {
                a.batch_size = std::strtoull(argv[++i], nullptr, 10);
            }
            else if (arg == "--reserve" && i + 1 < argc) {
                a.reserve_records = std::strtoull(argv[++i], nullptr, 10);
            }
            else if (a.config_path == "data/config.json") {
                // fallback: first non-flag argument as config path
                a.config_path = arg;
            }
        }
        return a;
    }

} // namespace app
