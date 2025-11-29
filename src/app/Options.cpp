#include "Options.hpp"
#include <stdexcept>
#include <iostream>
#include <ctime>

namespace {

    std::chrono::system_clock::time_point parse_time(const std::string& s) {
        // Very small parser: "YYYY-MM-DDTHH:MM" in UTC, e.g. 2024-01-01T08:00
        std::tm tm{};
        if (s.size() != 16 || s[10] != 'T')
            throw std::runtime_error("Bad time format (expected YYYY-MM-DDTHH:MM): " + s);

        tm.tm_year = std::stoi(s.substr(0, 4)) - 1900;
        tm.tm_mon = std::stoi(s.substr(5, 2)) - 1;
        tm.tm_mday = std::stoi(s.substr(8, 2));
        tm.tm_hour = std::stoi(s.substr(11, 2));
        tm.tm_min = std::stoi(s.substr(14, 2));
        tm.tm_sec = 0;

        std::time_t tt = timegm(&tm);
        return std::chrono::system_clock::from_time_t(tt);
    }

    void print_help() {
        std::cout <<
            "Usage: citysense [options]\n"
            "  --input FILE        CSV input (repeatable)\n"
            "  --zone  ID          zone filter (repeatable)\n"
            "  --from  TS          start time (YYYY-MM-DDTHH:MM)\n"
            "  --to    TS          end time   (YYYY-MM-DDTHH:MM)\n"
            "  --mode  csv|sim     ingestion from CSV files or simulator\n"
            "  --batch N           batch size for Reader/Simulator\n"
            "  --reserve N         pre-reserve N rows in aggregator/window\n"
            "  --seed N            simulator seed (when --mode sim)\n"
            "  -h, --help          show this help\n";
    }

} // namespace

namespace app {

    Options parse_args(int argc, char** argv) {
        Options opt;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            auto need_value = [&](const char* name) {
                if (i + 1 >= argc)
                    throw std::runtime_error(std::string("Missing value for ") + name);
                return std::string{ argv[++i] };
                };

            if (arg == "-h" || arg == "--help") {
                print_help();
                std::exit(0);
            }
            else if (arg == "--input") {
                opt.inputs.push_back(need_value("--input"));
            }
            else if (arg == "--zone") {
                opt.zones.push_back(std::stoi(need_value("--zone")));
            }
            else if (arg == "--from") {
                opt.from = parse_time(need_value("--from"));
            }
            else if (arg == "--to") {
                opt.to = parse_time(need_value("--to"));
            }
            else if (arg == "--mode") {
                auto v = need_value("--mode");
                if (v == "csv")      opt.mode = IngestMode::Csv;
                else if (v == "sim") opt.mode = IngestMode::Simulator;
                else throw std::runtime_error("Unknown --mode: " + v);
            }
            else if (arg == "--batch") {
                opt.batch_size = static_cast<std::size_t>(std::stoul(need_value("--batch")));
            }
            else if (arg == "--reserve") {
                opt.reserve_rows = static_cast<std::size_t>(std::stoul(need_value("--reserve")));
            }
            else if (arg == "--seed") {
                opt.sim_seed = std::stoi(need_value("--seed"));
            }
            else {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }

        return opt;
    }

} // namespace app
