#pragma once

namespace core {

    struct Window; // match definition in Window.hpp

    // Minimal detector contract. Students can extend with return values or reports.
    class Detector {
    public:
        virtual ~Detector() = default;
        virtual void detect(const Window& /*w*/) = 0;
    };

} // namespace core
