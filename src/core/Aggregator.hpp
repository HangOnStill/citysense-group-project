#pragma once
#include <unordered_map>
#include "Window.hpp"

namespace core {

// Public summary contract used by tests.
struct Summary {
    int total_count{0};
    std::unordered_map<int,int> by_zone; // zone_id -> count
};

class Aggregator {
public:
    explicit Aggregator(int /*window_minutes*/) {}
    template <typename Range>
    void consume(const Range& /*r*/) {
        // TODO: append into window, maintain size, update indexes
        ++dummy_count_;
    }
    const Window& current_window_view() const { return window_; }
    Summary summary() const {
        Summary s; s.total_count = dummy_count_; return s;
    }
private:
    Window window_;
    int dummy_count_{0};
};

} // namespace core
