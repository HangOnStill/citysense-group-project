#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include "app/Cli.hpp"
#include "app/Config.hpp"
#include "core/Aggregator.hpp"
#include "detectors/AirAlert.hpp"
#include "detectors/Finding.hpp"
#include "detectors/NoiseSpike.hpp"
#include "detectors/TrafficCongestion.hpp"
#include "export/ConsolePrinter.hpp"
#include "export/JsonExporter.hpp"
#include "io/ReaderCSV.hpp"
#include "io/ReaderJSON.hpp"
#include "model/SensorRecord.hpp"
#include "export/ConsoleDetailsPrinter.hpp"


namespace {

    // --- Small JSON helpers for config ------------------------------------------

    std::string read_file_to_string(const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) {
            return {};
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    std::optional<std::string> extract_string(const std::string& text,
        std::string_view key) {
        const std::string pattern = "\"" + std::string(key) + "\"";
        auto pos = text.find(pattern);
        if (pos == std::string::npos) return std::nullopt;

        pos = text.find(':', pos + pattern.size());
        if (pos == std::string::npos) return std::nullopt;

        ++pos; // past ':'
        while (pos < text.size() &&
            std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
        if (pos >= text.size() || text[pos] != '"') return std::nullopt;

        ++pos; // skip opening quote
        auto end = text.find('"', pos);
        if (end == std::string::npos) return std::nullopt;

        return text.substr(pos, end - pos);
    }

    std::optional<std::int64_t> extract_int64(const std::string& text,
        std::string_view key) {
        const std::string pattern = "\"" + std::string(key) + "\"";
        auto pos = text.find(pattern);
        if (pos == std::string::npos) return std::nullopt;

        pos = text.find(':', pos + pattern.size());
        if (pos == std::string::npos) return std::nullopt;

        ++pos; // past ':'
        while (pos < text.size() &&
            std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
        if (pos >= text.size()) return std::nullopt;

        auto start = pos;
        while (pos < text.size() &&
            (std::isdigit(static_cast<unsigned char>(text[pos])) ||
                text[pos] == '-' || text[pos] == '+')) {
            ++pos;
        }
        if (start == pos) return std::nullopt;

        try {
            auto value = std::stoll(text.substr(start, pos - start));
            return value;
        }
        catch (...) {
            return std::nullopt;
        }
    }

    std::optional<double> extract_double(const std::string& text,
        std::string_view key) {
        const std::string pattern = "\"" + std::string(key) + "\"";
        auto pos = text.find(pattern);
        if (pos == std::string::npos) return std::nullopt;

        pos = text.find(':', pos + pattern.size());
        if (pos == std::string::npos) return std::nullopt;

        ++pos; // past ':'
        while (pos < text.size() &&
            std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
        if (pos >= text.size()) return std::nullopt;

        auto start = pos;
        while (pos < text.size() &&
            (std::isdigit(static_cast<unsigned char>(text[pos])) ||
                text[pos] == '-' || text[pos] == '+' ||
                text[pos] == '.' || text[pos] == 'e' || text[pos] == 'E')) {
            ++pos;
        }
        if (start == pos) return std::nullopt;

        try {
            auto value = std::stod(text.substr(start, pos - start));
            return value;
        }
        catch (...) {
            return std::nullopt;
        }
    }

    std::vector<std::string> extract_string_array(const std::string& text,
        std::string_view key) {
        std::vector<std::string> result;
        const std::string pattern = "\"" + std::string(key) + "\"";
        auto pos = text.find(pattern);
        if (pos == std::string::npos) return result;

        pos = text.find('[', pos + pattern.size());
        if (pos == std::string::npos) return result;

        ++pos; // past '['
        while (pos < text.size()) {
            while (pos < text.size() &&
                std::isspace(static_cast<unsigned char>(text[pos]))) {
                ++pos;
            }
            if (pos >= text.size() || text[pos] == ']') break;

            if (text[pos] != '"') {
                ++pos;
                continue;
            }
            ++pos; // skip opening quote
            auto end = text.find('"', pos);
            if (end == std::string::npos) break;

            result.push_back(text.substr(pos, end - pos));
            pos = end + 1;
        }

        return result;
    }

    // Load just the fields we care about from config.json into app::Config.
    app::Config load_config(const std::string& path) {
        app::Config cfg;
        const auto json = read_file_to_string(path);
        if (json.empty()) {
            // Fallback to a reasonable default input.
            if (cfg.inputs.empty()) {
                cfg.inputs = { "data/air.csv" };
            }
            return cfg;
        }

        if (auto inputs = extract_string_array(json, "inputs"); !inputs.empty()) {
            cfg.inputs = std::move(inputs);
        }
        else if (cfg.inputs.empty()) {
            cfg.inputs = { "data/air.csv" };
        }

        if (auto w = extract_int64(json, "window_minutes")) {
            cfg.window_minutes = static_cast<int>(*w);
        }

        if (auto t_speed = extract_double(json, "traffic_speed_threshold")) {
            cfg.traffic_speed_threshold = *t_speed;
        }
        if (auto consec = extract_int64(json, "consec_minutes")) {
            cfg.consec_minutes = static_cast<int>(*consec);
        }
        if (auto pm25 = extract_double(json, "pm25_threshold")) {
            cfg.pm25_threshold = *pm25;
        }
        if (auto db = extract_double(json, "db_threshold")) {
            cfg.db_threshold = *db;
        }
        if (auto noise_count = extract_int64(json, "noise_count_threshold")) {
            cfg.noise_count_threshold = static_cast<int>(*noise_count);
        }

        if (auto path_out = extract_string(json, "out_path")) {
            cfg.out_path = *path_out;
        }

        return cfg;
    }

    bool ends_with(const std::string& s, std::string_view suffix) {
        if (s.size() < suffix.size()) return false;
        return std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
    }

    // Load all records from CSV and JSON readers, then sort by timestamp.
    std::vector<model::SensorRecord>
        load_all_records(io::ReaderCSV* csv_reader,
            io::ReaderJSON* json_reader) {
        std::vector<model::SensorRecord> records;

        // NOTE: ReaderCSV in this project re-reads the whole file from the
        // beginning on each call and does NOT maintain internal streaming state.
        // Therefore, we must call next_batch() ONLY ONCE per CSV reader and
        // choose a large enough max_rows to cover the entire file.
        if (csv_reader != nullptr) {
            auto batch = csv_reader->next_batch(1'000'000); // big upper bound
            std::move(batch.begin(), batch.end(), std::back_inserter(records));
        }

        // ReaderJSON *does* maintain streaming state across calls,
        // so we can safely loop until it returns an empty batch.
        if (json_reader != nullptr) {
            for (;;) {
                auto batch = json_reader->next_batch(1024);
                if (batch.empty()) break;
                std::move(batch.begin(), batch.end(), std::back_inserter(records));
            }
        }

        std::sort(records.begin(), records.end(),
            [](const model::SensorRecord& a, const model::SensorRecord& b) {
                return a.ts < b.ts;
            });

        return records;
    }

} // namespace

int main(int argc, char** argv) {
    try {
        const auto cli = app::parse_cli(argc, argv);
        app::Config cfg = load_config(cli.config_path);

        if (cfg.inputs.empty()) {
            std::cerr << "No input files specified in config.\n";
            return 1;
        }

        // Split inputs by extension.
        std::vector<std::string> csv_inputs;
        std::vector<std::string> json_inputs;
        csv_inputs.reserve(cfg.inputs.size());
        json_inputs.reserve(cfg.inputs.size());

        for (const auto& path : cfg.inputs) {
            if (ends_with(path, ".json")) {
                json_inputs.push_back(path);
            }
            else {
                csv_inputs.push_back(path);
            }
        }

        std::unique_ptr<io::ReaderCSV> csv_reader;
        std::unique_ptr<io::ReaderJSON> json_reader;

        if (!csv_inputs.empty()) {
            csv_reader = std::make_unique<io::ReaderCSV>(csv_inputs);
        }
        if (!json_inputs.empty()) {
            json_reader = std::make_unique<io::ReaderJSON>(json_inputs);
        }

        auto all_records = load_all_records(csv_reader.get(),
            json_reader.get());
        if (all_records.empty()) {
            std::cerr << "No sensor records loaded from inputs.\n";
            return 0;
        }

        // Aggregate everything in one pass.
        core::Aggregator aggregator(cfg.window_minutes);
        aggregator.consume(all_records);

        const auto summary = aggregator.summary();

        // Detectors on the final window.
        const auto& window = aggregator.current_window_view();
        detectors::TrafficCongestion traffic(cfg.traffic_speed_threshold,
            cfg.consec_minutes);
        detectors::AirAlert air(cfg.pm25_threshold);
        detectors::NoiseSpike noise(cfg.db_threshold,
            cfg.noise_count_threshold);

        traffic.detect(window);
        air.detect(window);
        noise.detect(window);

        std::vector<detectors::Finding> all_findings;

        for (const auto& f : traffic.latest_findings()) all_findings.push_back(f);
        for (const auto& f : air.latest_findings())     all_findings.push_back(f);
        for (const auto& f : noise.latest_findings())   all_findings.push_back(f);


        // Outputs.
        export_::ConsolePrinter console;
        export_::JsonExporter json_exporter(cfg.out_path);

        console.emit(summary);
        json_exporter.emit(summary);
        // Later: extend JsonExporter to include detector findings if desired.

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 1;
    }
}
