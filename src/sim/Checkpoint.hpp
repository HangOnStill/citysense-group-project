#pragma once
#include <string>

namespace sim {

// Minimal placeholder for persisting simulator state.
struct Checkpoint {
    std::string path{"checkpoint.bin"};
    // TODO: save/restore small state (clock, offsets).
};

} // namespace sim
