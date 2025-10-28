#pragma once
#include <string>

namespace app {

struct CliArgs {
    std::string config_path{"data/config.json"};
};

inline CliArgs parse_cli(int argc, char** argv) {
    CliArgs a;
    if (argc > 1) a.config_path = argv[1];
    return a;
}

} // namespace app
