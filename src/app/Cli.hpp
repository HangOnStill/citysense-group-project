#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <cstdlib>

namespace app {

    struct CliArgs {
        std::string config_path{ "data/config.json" };
        std::optional<std::size_t> batch;
        std::optional<std::size_t> reserve;
    };

    // Very small hand-rolled parser:
    //   citysense [config.json] [--batch N] [--reserve N]
    inline CliArgs parse_cli(int argc, char** argv) {
        CliArgs a;

        if (argc > 1) {
            // First non-flag argument is treated as config path
            std::string_view first{ argv[1] };
            if (!first.empty() && first[0] != '-') {
                a.config_path = std::string(first);
            }
        }

        for (int i = 1; i < argc; ++i) {
            std::string_view arg{ argv[i] };
            if (arg == "--batch" && i + 1 < argc) {
                a.batch = static_cast<std::size_t>(std::strtoull(argv[++i], nullptr, 10));
            }
            else if (arg == "--reserve" && i + 1 < argc) {
                a.reserve = static_cast<std::size_t>(std::strtoull(argv[++i], nullptr, 10));
            }
        }

        return a;
    }

} // namespace app
