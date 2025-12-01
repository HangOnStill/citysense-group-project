#pragma once
#include <vector>
#include "core/Finding.hpp"

namespace core {

    class Window;       // ← change 'struct' to 'class'

    class Detector {
    public:
        virtual ~Detector() = default;
        virtual std::vector<Finding> detect(const Window&) = 0;
    };

} // namespace core
