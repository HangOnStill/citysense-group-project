// src/core/Detector.hpp
#pragma once
#include <vector>

namespace core {

    struct Window;   // defined in Window.hpp
    struct Finding;  // defined in Finding.hpp

    // Minimal detector contract.
    class Detector {
    public:
        virtual ~Detector() = default;
        virtual std::vector<Finding> detect(const Window& /*w*/) = 0;
    };

} // namespace core