// dev/demo_aggregator.cpp
#include <iostream>
#include <vector>

#include "app/Options.hpp"
#include "io/ReaderCSV.hpp"
#include "core/Aggregator.hpp"
#include "model/SensorRecord.hpp"

int main(int argc, char** argv) {
    app::Options opt;
    try {
        opt = app::parse_args(argc, argv);
    }
    catch (const std::exception& ex) {
        std::cerr << "Argument error: " << ex.what() << "\n";
        return 1;
    }

    core::Aggregator agg{ opt.window_minutes };  // or fixed 5
    if (opt.reserve_rows > 0) {
        agg.reserve(opt.reserve_rows);
    }

    io::ReaderCSV reader{ opt.inputs };
    for (;;) {
        auto batch = reader.next_batch(opt.batch_size);
        if (batch.empty()) break;
        agg.consume(batch);
    }

    auto sum = agg.summary();
    std::cout << "Total ingested rows: " << sum.total_count << "\n";
    std::cout << "Per zone:\n";
    for (auto& [zone, count] : sum.by_zone) {
        std::cout << "  zone " << zone << ": " << count << "\n";
    }
    return 0;
}
